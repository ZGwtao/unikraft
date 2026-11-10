/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef CARRELS_SBUF_H
#define CARRELS_SBUF_H

#include <uk/netdev.h>
#include <uk/sddf.h>

struct uk_netdev_rx_queue {
	net_queue_handle_t handle;
	uint8_t *data;
	microkit_channel notify_id;
	uk_netdev_alloc_rxpkts alloc_rxpkts;
	void *alloc_rxpkts_argp;
};

struct uk_netdev_tx_queue {
	net_queue_handle_t handle;
	uint8_t *data;
	microkit_channel notify_id;
};

/* RX descriptor ownership remains with the netbuf until its destructor. */
void carrels_rx_init(struct uk_netdev_rx_queue *queue);
int carrels_rx_one(struct uk_netbuf **pkt);

/* TX uses the existing copy path; this interface only moves its code. */
int carrels_tx_send_copy(struct uk_netdev_tx_queue *queue,
			const struct uk_netbuf *pkt, size_t pkt_len);
int carrels_tx_has_available(struct uk_netdev_tx_queue *queue);

#endif /* CARRELS_SBUF_H */
