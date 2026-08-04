/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SDDF_NETDEV_H__
#define __SDDF_NETDEV_H__

#include <uk/sddf.h>

int uk_carrels_netdev_register(const net_client_config_t *config,
			       net_queue_handle_t *rxq,
			       net_queue_handle_t *txq);

#endif /* __SDDF_NETDEV_H__  */