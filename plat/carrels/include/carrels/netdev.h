/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __CARRELS_NETDEV_H__
#define __CARRELS_NETDEV_H__

#include <uk/microkit.h>

int uk_carrels_netdev_register(const net_client_config_t *config,
			       net_queue_handle_t *rxq,
			       net_queue_handle_t *txq);

#endif /* __CARRELS_NETDEV_H__ */