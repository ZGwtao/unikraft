/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <uk/assert.h>
#include <uk/netbuf.h>
#include <uk/sddf.h>

#include "sbuf.h"

#define MAX_TX_BUFFERS 1024
#define NO_TX_BUFFER UINT16_MAX

typedef struct tx_slot {
	/* Live client references, including the owner returned by acquire. */
	uint16_t references;
	/* Link in the local ready queue; valid only while the slot is ready. */
	uint16_t next_ready;
	/* The descriptor has been published but not returned by sDDF. */
	bool in_flight;
	/* Identifies the netbuf to which this slot was loaned. */
	struct uk_netbuf *owner;
} tx_slot_t;

/* The driver currently registers one sDDF network client. */
static struct uk_netdev_tx_queue *tx_queue;
static tx_slot_t tx_slots[MAX_TX_BUFFERS];

/*
 * Keep track of available TX buffers dequeued from the TX free queue.
 * The available buffers form a FIFO ready queue. Each ready slot has zero
 * references and is not occupied by an outstanding I/O request.
 */
static struct {
	uint16_t head;
	uint16_t tail;
} ready_tx = {NO_TX_BUFFER, NO_TX_BUFFER};

static uint16_t tx_index(uint64_t offset)
{
	return offset / NET_BUFFER_SIZE;
}

/* FIFO order lets returned slots rotate instead of reusing the newest one. */
static void tx_ready_enqueue(uint16_t index)
{
	tx_slots[index].next_ready = NO_TX_BUFFER;
	if (ready_tx.tail == NO_TX_BUFFER)
		ready_tx.head = index;
	else
		tx_slots[ready_tx.tail].next_ready = index;

	ready_tx.tail = index;
}

static uint16_t tx_ready_dequeue(void)
{
	uint16_t index = ready_tx.head;

	if (index == NO_TX_BUFFER)
		return index;

	ready_tx.head = tx_slots[index].next_ready;
	if (ready_tx.head == NO_TX_BUFFER)
		ready_tx.tail = NO_TX_BUFFER;

	return index;
}

static void tx_drop_reference(uint16_t index)
{
	tx_slot_t *slot = &tx_slots[index];

	UK_ASSERT(slot->references > 0);
	/* A slot becomes ready only after the last reference is gone and sDDF
	 * has returned its descriptor. Either event may happen first. */
	if (--slot->references == 0 && !slot->in_flight)
		tx_ready_enqueue(index);
}

/* Drain returned descriptors into the local ready queue when unreferenced. */
static void tx_reclaim(void)
{
	net_buff_desc_t desc;

	while (net_dequeue_free(&tx_queue->handle, &desc) == 0) {
		uint64_t offset = desc.io_or_offset;
		uint16_t index;

		if (offset % NET_BUFFER_SIZE ||
		    offset / NET_BUFFER_SIZE >= tx_queue->handle.capacity)
			UK_CRASH("CARRELS: invalid TX descriptor");

		index = tx_index(offset);

		/* Seeing the descriptor in the free queue means sDDF has returned it.
		 * A live netbuf reference still prevents the slot from being reused. */
		 tx_slots[index].in_flight = false;
		if (tx_slots[index].references == 0)
			tx_ready_enqueue(index);
	}
}

int carrels_tx_init(struct uk_netdev_tx_queue *queue)
{
	if (queue->handle.capacity > MAX_TX_BUFFERS)
		return -EINVAL;

	tx_queue = queue;
	tx_reclaim();
	return 0;
}

static void carrels_netbuf_copy(void *dst, const struct uk_netbuf *pkt)
{
	const struct uk_netbuf *seg;
	uint8_t *cursor = dst;

	for (seg = pkt; seg != NULL; seg = seg->next) {
		memcpy(cursor, seg->data, seg->len);
		cursor += seg->len;
	}
}

static int carrels_tx_dequeue_buffer(net_buff_desc_t *desc)
{
	uint16_t index;

	/* Reclaim descriptors returned by sDDF before reporting no free slot. */
	if (ready_tx.head == NO_TX_BUFFER)
		tx_reclaim();

	/* If the ready queue is still empty, all slots are held by netbufs or
	 * remain in flight with the network component. */
	index = tx_ready_dequeue();
	if (index == NO_TX_BUFFER)
		return -1;

	UK_ASSERT(tx_slots[index].references == 0 &&
		  !tx_slots[index].in_flight && !tx_slots[index].owner);

	tx_slots[index].references = 1;

	desc->io_or_offset = (uint64_t)index * NET_BUFFER_SIZE;
	desc->len = 0;
	desc->oid = 0;

	return 0;
}

static void carrels_tx_prepare_buffer(struct uk_netdev_tx_queue *queue,
				      net_buff_desc_t *desc,
				      const struct uk_netbuf *pkt,
				      size_t pkt_len)
{
	uint8_t *dst;

	UK_ASSERT(desc->io_or_offset % NET_BUFFER_SIZE == 0);
	UK_ASSERT(desc->io_or_offset <
		  (uint64_t)queue->handle.capacity * NET_BUFFER_SIZE);

	dst = queue->data + desc->io_or_offset;
	carrels_netbuf_copy(dst, pkt);
	desc->len = (uint16_t)pkt_len;
}

static void carrels_tx_submit_buffer(struct uk_netdev_tx_queue *queue,
				     net_buff_desc_t desc)
{
	/* The active queue has a slot when a free descriptor was dequeued. */
	if (net_enqueue_active(&queue->handle, desc) != 0)
		UK_CRASH("CARRELS: failed to submit TX descriptor");

	if (net_require_signal_active(&queue->handle)) {
		net_cancel_signal_active(&queue->handle);
		microkit_notify(queue->notify_id);
	}
}

static int tx_publish(struct uk_netdev_tx_queue *queue, net_buff_desc_t desc)
{
	uint16_t index = tx_index(desc.io_or_offset);

	if (net_queue_full_active(&queue->handle) || tx_slots[index].in_flight)
		return -1;

	tx_slots[index].in_flight = true;
	carrels_tx_submit_buffer(queue, desc);
	return 0;
}

int carrels_tx_has_available(struct uk_netdev_tx_queue *queue)
{
	if (ready_tx.head == NO_TX_BUFFER)
		tx_reclaim();

	return ready_tx.head != NO_TX_BUFFER &&
		!net_queue_full_active(&queue->handle);
}

int carrels_tx_send_copy(struct uk_netdev_tx_queue *queue,
			const struct uk_netbuf *pkt, size_t pkt_len)
{
	net_buff_desc_t desc;
	int published;

	/* pkt may be heap-backed. Only the acquired destination descriptor must
	 * belong to the sDDF TX region; prepare_buffer checks that offset. */
	if (carrels_tx_dequeue_buffer(&desc) != 0)
		return 0;

	carrels_tx_prepare_buffer(queue, &desc, pkt, pkt_len);
	published = tx_publish(queue, desc);

	tx_drop_reference(tx_index(desc.io_or_offset));
	return published == 0;
}

static void tx_loan_dtor(struct uk_netbuf *nb)
{
	uintptr_t base = (uintptr_t)tx_queue->data;
	uintptr_t addr = (uintptr_t)nb->buf;
	uint16_t index;

	UK_ASSERT(addr >= base && addr - base <
		  (uintptr_t)tx_queue->handle.capacity * NET_BUFFER_SIZE);
	UK_ASSERT((addr - base) % NET_BUFFER_SIZE == 0);

	index = tx_index(addr - base);
	UK_ASSERT(tx_slots[index].owner == nb);
	tx_slots[index].owner = NULL;

	tx_drop_reference(index);
}

struct uk_netbuf *carrels_tx_alloc(struct uk_netdev_tx_queue *queue,
				   size_t capacity, uint16_t headroom)
{
	struct uk_netbuf *nb;
	net_buff_desc_t desc;

	/* sDDF descriptors address the beginning of each TX slot. */
	if (!queue->alloc_txbuf || capacity > NET_BUFFER_SIZE || headroom != 0 ||
	    carrels_tx_dequeue_buffer(&desc) != 0)
		return NULL;

	nb = queue->alloc_txbuf(queue->alloc_txbuf_argp,
				queue->data + desc.io_or_offset,
				NET_BUFFER_SIZE, headroom, tx_loan_dtor);
	if (!nb) {
		tx_drop_reference(tx_index(desc.io_or_offset));
		return NULL;
	}

	tx_slots[tx_index(desc.io_or_offset)].owner = nb;
	return nb;
}

/* Identify a loan without publishing its descriptor. */
static enum carrels_tx_loan_result tx_loan_offset(
	struct uk_netdev_tx_queue *queue,
	const struct uk_netbuf *pkt, uint64_t *offset)
{
	uintptr_t tx_region_base = (uintptr_t)queue->data;
	uintptr_t pkt_buf_addr = (uintptr_t)pkt->buf;
	uint16_t index;

	// When the pkt is in a heap-based buffer
	if (pkt_buf_addr < tx_region_base ||
	    pkt_buf_addr - tx_region_base >=
	    (uintptr_t)queue->handle.capacity * NET_BUFFER_SIZE)
		return CARRELS_TX_COPY_REQUIRED;

	// When the in-sddf buffer has invalid offset
	if ((pkt_buf_addr - tx_region_base) % NET_BUFFER_SIZE != 0)
		return CARRELS_TX_LOAN_REJECTED;

	index = tx_index(pkt_buf_addr - tx_region_base);
	// If pkt->next is set, the packet has more data in another netbuf.
	// Publishing this TX buffer alone would omit that data, as send_loan
	// can transmit/process single netbuf per invocation.
	if (tx_slots[index].owner != pkt || pkt->next ||
	    pkt->data != pkt->buf || pkt->buflen != NET_BUFFER_SIZE)
		return CARRELS_TX_LOAN_REJECTED;

	*offset = pkt_buf_addr - tx_region_base;
	return CARRELS_TX_LOAN_VALID;
}

enum carrels_tx_loan_result carrels_tx_send_loan(
	struct uk_netdev_tx_queue *queue, struct uk_netbuf *pkt, size_t len)
{
	net_buff_desc_t desc;
	enum carrels_tx_loan_result r;

	r = tx_loan_offset(queue, pkt, &desc.io_or_offset);
	if (r != CARRELS_TX_LOAN_VALID)
		return r;

	desc.len = (uint16_t)len;
	desc.oid = 0;

	if (tx_publish(queue, desc) != 0)
		return CARRELS_TX_LOAN_REJECTED;

	return CARRELS_TX_LOAN_SENT;
}
