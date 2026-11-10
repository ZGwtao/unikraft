/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdint.h>
#include <string.h>

#include <uk/assert.h>
#include <uk/netbuf.h>
#include <uk/sddf.h>

#include "sbuf.h"

static void carrels_netbuf_copy(void *dst, const struct uk_netbuf *pkt)
{
	const struct uk_netbuf *seg;
	uint8_t *cursor = dst;

	for (seg = pkt; seg != NULL; seg = seg->next) {
		memcpy(cursor, seg->data, seg->len);
		cursor += seg->len;
	}
}

static int carrels_tx_dequeue_buffer(struct uk_netdev_tx_queue *queue,
				    net_buff_desc_t *desc)
{
	if (net_queue_full_active(&queue->handle))
		return -1;
	return net_dequeue_free(&queue->handle, desc);
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

int carrels_tx_has_available(struct uk_netdev_tx_queue *queue)
{
	return !net_queue_empty_free(&queue->handle) &&
		!net_queue_full_active(&queue->handle);
}

int carrels_tx_send_copy(struct uk_netdev_tx_queue *queue,
			const struct uk_netbuf *pkt, size_t pkt_len)
{
	net_buff_desc_t desc;

	if (carrels_tx_dequeue_buffer(queue, &desc) != 0)
		return 0;
	carrels_tx_prepare_buffer(queue, &desc, pkt, pkt_len);
	carrels_tx_submit_buffer(queue, desc);
	return 1;
}
