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
	uk_netdev_alloc_txbuf alloc_txbuf;
	void *alloc_txbuf_argp;
};

/* RX descriptor ownership remains with the netbuf until its destructor. */
void carrels_rx_init(struct uk_netdev_rx_queue *queue);
int carrels_rx_one(struct uk_netbuf **pkt);

/* All TX free descriptors pass through this module's ready queue. */
int carrels_tx_init(struct uk_netdev_tx_queue *queue);
struct uk_netbuf *carrels_tx_alloc(struct uk_netdev_tx_queue *queue,
				   size_t capacity, uint16_t headroom);

enum carrels_tx_loan_result {
	CARRELS_TX_LOAN_REJECTED = -1,
	CARRELS_TX_COPY_REQUIRED = 0,
	CARRELS_TX_LOAN_VALID = 1,
	CARRELS_TX_LOAN_SENT = 2,
};

/* Try to publish a netbuf backed by this queue's TX slot.
 * CARRELS_TX_LOAN_VALID is used internally during ownership validation.
 * CARRELS_TX_LOAN_SENT: descriptor published; caller must release pkt.
 * CARRELS_TX_COPY_REQUIRED: pkt is outside this TX region; use the copy path.
 * CARRELS_TX_LOAN_REJECTED: pkt points into this TX region but is invalid,
 * or the active queue cannot accept it now; nothing was published and the
 * caller retains pkt.
 */
enum carrels_tx_loan_result carrels_tx_send_loan(
	struct uk_netdev_tx_queue *queue, struct uk_netbuf *pkt, size_t len);
/* Returns 1 if sent and 0 if no TX slot is available. */
int carrels_tx_send_copy(struct uk_netdev_tx_queue *queue,
			const struct uk_netbuf *pkt, size_t pkt_len);
int carrels_tx_has_available(struct uk_netdev_tx_queue *queue);

#endif /* CARRELS_SBUF_H */
