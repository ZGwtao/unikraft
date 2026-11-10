/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdint.h>
#include <string.h>

#include <uk/assert.h>
#include <uk/netbuf.h>
#include <uk/print.h>
#include <uk/sddf.h>

#include "sbuf.h"

/* The driver currently registers one sDDF network client. */
static struct uk_netdev_rx_queue *rx_queue;

void carrels_rx_init(struct uk_netdev_rx_queue *queue)
{
	rx_queue = queue;
}

static int carrels_rx_buffer_valid(const net_buff_desc_t *desc, const __u64 capacity)
{
	/* Validate payload length */
	if (desc->len < UK_ETH_HDR_UNTAGGED_LEN ||
	    desc->len > NET_BUFFER_SIZE) {
		uk_pr_info("CARRELS RX: invalid frame length %u\n", desc->len);
		return 0;
	}
	/* Validate payload alignment and range */
	return desc->io_or_offset % NET_BUFFER_SIZE == 0 &&
		desc->io_or_offset < capacity * NET_BUFFER_SIZE;
}

static struct uk_netbuf *
carrels_rx_alloc_netbuf(struct uk_netdev_rx_queue *queue)
{
	struct uk_netbuf *nb = NULL;

	if (unlikely(queue->alloc_rxpkts(queue->alloc_rxpkts_argp, &nb, 1) != 1 ||
		     !nb)) {
		uk_pr_info("CARRELS RX: netbuf allocation failed\n");
		return NULL;
	}
	return nb;
}

static void carrels_rx_return_buffer(struct uk_netdev_rx_queue *queue,
				     net_buff_desc_t desc)
{
	desc.len = 0;
	if (net_enqueue_free(&queue->handle, desc) != 0)
		UK_CRASH("CARRELS: failed to recycle RX descriptor");

	if (net_require_signal_free(&queue->handle)) {
		net_cancel_signal_free(&queue->handle);
		microkit_notify(queue->notify_id);
	}
}

static void carrels_rx_netbuf_dtor(struct uk_netbuf *nb)
{
	/* This driver currently registers one sDDF network client. */
	struct uk_netdev_rx_queue *queue = rx_queue;
	uintptr_t base = (uintptr_t)queue->data;
	uintptr_t addr = (uintptr_t)nb->buf;
	net_buff_desc_t desc = { 0 };

	UK_ASSERT(addr >= base);
	UK_ASSERT(addr - base <
		  (uintptr_t)queue->handle.capacity * NET_BUFFER_SIZE);
	UK_ASSERT((addr - base) % NET_BUFFER_SIZE == 0);

	desc.io_or_offset = addr - base;

	carrels_rx_return_buffer(queue, desc);
}

static void carrels_rx_bind_netbuf(struct uk_netbuf *nb, void *buf,
				  uint16_t frame_len)
{
	nb->buf = buf;
	nb->data = buf;
	nb->buflen = NET_BUFFER_SIZE;
	nb->len = frame_len;
	nb->dtor = carrels_rx_netbuf_dtor;
}

static struct uk_netbuf *
carrels_rx_consume_buffer(struct uk_netdev_rx_queue *queue,
			  const net_buff_desc_t *desc, int *loaned)
{
	struct uk_netbuf *nb;
	uint8_t *src;

	/* Mark the netbuf state */
	*loaned = 0;

	if (unlikely(!carrels_rx_buffer_valid(desc, queue->handle.capacity)))
		return NULL;

	nb = carrels_rx_alloc_netbuf(queue);
	if (!nb)
		return NULL;

	src = queue->data + desc->io_or_offset;

	// @gt We use (void *data) in 'uk_netbuf' to denote the state of whether
	//     the netbuf is allocated with a rx buffer already or not.
	//     Zero (NULL) means the netbuf is descriptor only, and we can avoid
	//     an additional copy below.
	if (!nb->data) {
		if (unlikely(nb->buf || nb->buflen || nb->dtor)) {
			uk_netbuf_free(nb);
			return NULL;
		}
		/* The descriptor remains owned by this netbuf until its dtor. */
		carrels_rx_bind_netbuf(nb, src, desc->len);
		/* The netbuf points to sddf I/O buffer directly. */
		*loaned = 1;
		return nb;
	}

	// @gt Unfortunately, the alloc_rxpkt() returns with a rx buffer allocated,
	//     which means we will perform a data copy between the rx buffer of
	//     sddf and the rx buffer of uk_netbuf.
	if (unlikely(desc->len > nb->len)) {
		uk_pr_info(
			"CARRELS RX: frame %u exceeds capacity %u\n",
			(unsigned int)desc->len,
			(unsigned int)nb->len
		);
		uk_netbuf_free(nb);
		return NULL;
	}
	nb->len = desc->len;
	memcpy(nb->data, src, nb->len);
	return nb;
}

static int carrels_rx_status(struct uk_netdev_rx_queue *queue,
			    const struct uk_netbuf *pkt)
{
	int stx;

	/* No packet was delivered by this call (invalid frame or netbuf
	 * allocation failure). Other packets may still be queued. */
	if (!pkt)
		return 0;

	stx = UK_NETDEV_STATUS_SUCCESS;

	// More received packets are waiting in the RX queue.
	if (!net_queue_empty_active(&queue->handle))
		stx |= UK_NETDEV_STATUS_MORE;

	return stx;
}

int carrels_rx_one(struct uk_netbuf **pkt)
{
	// @gt ?? We use sddf buffer descriptor to describe an available
	//  network queue rx buffer for the Unikraft netdev. This is not
	//  clean, as a proper way is to use 'uk_netbuf' directly.
	//  Try to coordinate the uk_netbuf allocation with the net buf
	//  allocation process in the future.
	net_buff_desc_t desc;

	/* If set, the netbuf borrows the sDDF RX buffer and its destructor
	 * returns the descriptor to the free queue. */
	int loaned;

	if (net_dequeue_active(&rx_queue->handle, &desc) != 0)
		return 0;

	*pkt = carrels_rx_consume_buffer(rx_queue, &desc, &loaned);
	if (!loaned)
		carrels_rx_return_buffer(rx_queue, desc);

	/* Check whether there are more rx packages to handle. */
	return carrels_rx_status(rx_queue, *pkt);
}
