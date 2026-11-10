/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/init.h>
#include <uk/errptr.h>
#include <uk/netbuf.h>
#include <uk/netdev.h>
#include <uk/netdev_driver.h>
#include <uk/print.h>
#include <uk/sddf.h>

#include "sbuf.h"


UK_CTASSERT(MAC802_BYTES == UK_NETDEV_HWADDR_LEN);

extern net_client_config_t net_config;

typedef struct sddf_netdev {
	/* handle of the unikraft's network device */
	struct uk_netdev netdev;
	struct uk_hwaddr hwaddr;
	// @gt ?? why?
	struct uk_netdev_rx_queue rx_queue;
	struct uk_netdev_tx_queue tx_queue;

	/* sddf config for network connection */
	const net_client_config_t *config;

	// @gt ?? even if 'set_mtu' is provided, most Unikraft network
	//        drivers use static mtu which is given at boot, so no
	//        need to set a pair of 'max'+'normal' mtu like virtio,
	//        just keep a simple mtu here for bookkeeping purpose.
	__u16 mtu;
	/* RX promiscuous mode. */
	__u8 promisc; // like others, disabled by default.

} sddf_netdev_t;

/* virtual network device for Unikraft provided by sddf */
static sddf_netdev_t carrels_dev;

/* the 'netdev' attribute of sddf_netdev_t matches with 'dev' */
#define to_sddfnet_dev(dev) \
	__containerof(dev, sddf_netdev_t, netdev)

static int carrels_netdev_probe(struct uk_netdev *dev)
{
	sddf_netdev_t *cdev = to_sddfnet_dev(dev);

	UK_ASSERT(cdev->config);
	UK_ASSERT(cdev->rx_queue.handle.free);
	UK_ASSERT(cdev->tx_queue.handle.free);

	// @gt ?? this probe logic ensures that each sddf-backed
	//        network device is initialised once, which is helpful
	//        because the sddf virtualisers do not support re-init
	return 0;
}

static void carrels_netdev_info_get(struct uk_netdev *dev,
				    struct uk_netdev_info *info)
{
	UK_ASSERT(dev);
	UK_ASSERT(info);

	sddf_netdev_t *cdev = to_sddfnet_dev(dev);
	UK_ASSERT(cdev->config);

	memset(info, 0, sizeof(*info));

	/* 1 rx/tx queue pair per sddf network client */
	info->max_rx_queues = 1;
	info->max_tx_queues = 1;

	info->in_queue_pairs = 1;
	info->max_mtu = cdev->mtu;
	info->nb_encap_rx = 0;
	info->nb_encap_tx = 0;
	info->ioalign = 16;
	info->features = 0;
}

static int carrels_netdev_rxq_info_get(struct uk_netdev *dev,
				       uint16_t queue_id,
				       struct uk_netdev_queue_info *info)
{
	sddf_netdev_t *cdev = to_sddfnet_dev(dev);

	if (!info || queue_id != 0)
		return -EINVAL;

	memset(info, 0, sizeof(*info));

	info->nb_min = 1;
	info->nb_max = (uint16_t)cdev->rx_queue.handle.capacity;
	info->nb_align = 1;
	info->nb_is_power_of_two = 0;

	return 0;
}

static int carrels_netdev_txq_info_get(struct uk_netdev *dev,
				       uint16_t queue_id,
				       struct uk_netdev_queue_info *info)
{
	sddf_netdev_t *cdev = to_sddfnet_dev(dev);

	if (!info || queue_id != 0)
		return -EINVAL;

	memset(info, 0, sizeof(*info));

	info->nb_min = 1;
	info->nb_max = (uint16_t)cdev->tx_queue.handle.capacity;
	info->nb_align = 1;
	info->nb_is_power_of_two = 0;

	return 0;
}

// Invoked via ops->configure by uk_netdev_configure.
// It will changes the state of the netdev from unconfigured
// into 'UK_NETDEV_CONFIGURED'
// Ref: lib/uknetdev/netdev.c: uk_netdev_configure
static int carrels_netdev_configure(struct uk_netdev *dev,
				    const struct uk_netdev_conf *conf)
{
	(void)dev;
	// TODO
	// this function can be a checkpoint for resetting sddf
	// network virt-serving client if the unikernel is reboot.

	UK_ASSERT(conf);

	sddf_netdev_t *cdev = to_sddfnet_dev(dev);

	/* Ensure each rx/tx path has only one queue */
	if (conf->nb_rx_queues != 1 || conf->nb_tx_queues != 1)
		return -EINVAL;

	/* Disable large-receive-offload, as sddf API doesn't support */
	if (conf->lro)
		return -ENOTSUP;

	return 0;
}

static int carrels_netdev_queue_conf_valid(const void *conf, __u16 queue_id,
					  __u16 nb_desc, uint32_t capacity)
{
	/* sDDF queue capacity is fixed; zero requests the driver's default. */
	return conf && queue_id == 0 && // queue_id is fixed (only 1 queue per tx/rx)
		(nb_desc == 0 || nb_desc == capacity);
}

static struct uk_netdev_rx_queue *
carrels_netdev_rxq_configure(struct uk_netdev *dev,
			     __u16 queue_id, __u16 nb_desc,
			     struct uk_netdev_rxqueue_conf *conf)
{
	UK_ASSERT(conf);
	UK_ASSERT(dev);

	sddf_netdev_t *cdev = to_sddfnet_dev(dev);

	if (!carrels_netdev_queue_conf_valid(conf, queue_id, nb_desc,
					    cdev->rx_queue.handle.capacity))
		return ERR2PTR(-EINVAL);

	// User callback for allocating rx buffers (in shape of 'uk_netbuf')
	// so that the drivers can put the package into location specified
	// by the user (e.g., a network stack).
	// Ref: $(catalog)/repos/libs/lwip/uknetdev.c: netif_alloc_rxpkts
	if (!conf->alloc_rxpkts)
		return ERR2PTR(-EINVAL);

	cdev->rx_queue.alloc_rxpkts = conf->alloc_rxpkts;
	cdev->rx_queue.alloc_rxpkts_argp = conf->alloc_rxpkts_argp;

	return &cdev->rx_queue;
}

static struct uk_netdev_tx_queue *
carrels_netdev_txq_configure(struct uk_netdev *dev,
			     __u16 queue_id, __u16 nb_desc,
			     struct uk_netdev_txqueue_conf *conf)
{
	UK_ASSERT(conf);
	UK_ASSERT(dev);

	sddf_netdev_t *cdev = to_sddfnet_dev(dev);

	if (!carrels_netdev_queue_conf_valid(conf, queue_id, nb_desc,
					    cdev->tx_queue.handle.capacity))
		return ERR2PTR(-EINVAL);

	cdev->tx_queue.alloc_txbuf = conf->alloc_txbuf;
	cdev->tx_queue.alloc_txbuf_argp = conf->alloc_txbuf_argp;

	return &cdev->tx_queue;
}

// Invoked via ops->start by uk_netdev_start, which checks
// whether the netdev is configured (UK_NETDEV_CONFIGURED),
// and set the netdev afterwards.
// Once finishes, the netdev state goes to RUNNING
// Ref: lib/uknetdev/netdev.c: uk_netdev_start
static int carrels_netdev_start(struct uk_netdev *dev)
{
	(void)dev;
	// TODO
	// this can be extended as a checkpoint (for reset)
	// if the sddf virt supports re-configuration &
	// reconnection one day.
	return 0;
}

static const struct uk_hwaddr *
carrels_netdev_hwaddr_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	sddf_netdev_t *cdev = to_sddfnet_dev(dev);

	return &cdev->hwaddr;
}

static __u16 carrels_netdev_mtu_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	sddf_netdev_t *cdev = to_sddfnet_dev(dev);
	return cdev->mtu;
}

static unsigned carrels_netdev_promisc_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	sddf_netdev_t *cdev = to_sddfnet_dev(dev);
	return cdev->promisc;
}

static int carrels_rx_args_valid(struct uk_netdev *dev,
				 struct uk_netdev_rx_queue *queue,
				 struct uk_netbuf **pkt)
{
	return dev && queue && pkt &&
		queue == &to_sddfnet_dev(dev)->rx_queue;
}

static int carrels_netdev_rx_one(struct uk_netdev *dev,
		struct uk_netdev_rx_queue *queue, struct uk_netbuf **pkt)
{
	if (unlikely(!carrels_rx_args_valid(dev, queue, pkt)))
		return 0;
	return carrels_rx_one(pkt);
}

static size_t carrels_netbuf_len(const struct uk_netbuf *pkt)
{
	const struct uk_netbuf *seg;
	size_t total = 0;

	for (seg = pkt; seg != NULL; seg = seg->next)
		total += seg->len;

	return total;
}

static int carrels_tx_packet_valid(const struct uk_netbuf *pkt,
				   size_t *pkt_len)
{
	*pkt_len = carrels_netbuf_len(pkt);
	if (*pkt_len == 0 || *pkt_len > NET_BUFFER_SIZE) {
		uk_pr_err("CARRELS: invalid TX packet length %zu\n", *pkt_len);
		return 0;
	}
	return 1;
}

/* Report whether another TX buffer is available after this send. */
static int carrels_tx_status(struct uk_netdev_tx_queue *queue)
{
	int status = UK_NETDEV_STATUS_SUCCESS;

	if (carrels_tx_has_available(queue))
		status |= UK_NETDEV_STATUS_MORE;
	return status;
}

static struct uk_netbuf *carrels_netdev_tx_buf_alloc(struct uk_netdev *dev,
		struct uk_netdev_tx_queue *queue, size_t capacity,
		uint16_t headroom)
{
	(void)dev;
	return carrels_tx_alloc(queue, capacity, headroom);
}

static int carrels_tx_args_valid(struct uk_netdev *dev,
				 struct uk_netdev_tx_queue *queue,
				 struct uk_netbuf *pkt)
{
	return dev && queue && pkt &&
		queue == &to_sddfnet_dev(dev)->tx_queue;
}

static int carrels_netdev_tx_one(struct uk_netdev *dev,
				 struct uk_netdev_tx_queue *queue,
				 struct uk_netbuf *pkt)
{
	size_t pkt_len;
	enum carrels_tx_loan_result loan_result;

	if (unlikely(!carrels_tx_args_valid(dev, queue, pkt)))
		return 0;

	/* Ensure the whole netbuf chain fits in one sDDF TX buffer. */
	if (unlikely(!carrels_tx_packet_valid(pkt, &pkt_len)))
		return 0;

	/* Use the copy path only for a netbuf outside the sDDF TX region. */
	loan_result = carrels_tx_send_loan(queue, pkt, pkt_len);
	if (loan_result == CARRELS_TX_LOAN_REJECTED)
		return 0;

	if (loan_result == CARRELS_TX_COPY_REQUIRED &&
	    !carrels_tx_send_copy(queue, pkt, pkt_len))
		return 0;

	uk_netbuf_free(pkt);

	return carrels_tx_status(queue);
}

static int carrels_netdev_queues_init(sddf_netdev_t *dev)
{
	const net_client_config_t *cfg = dev->config;

	if (!cfg)
		return -EINVAL;

	net_queue_init(&dev->rx_queue.handle,
		       cfg->rx.free_queue.vaddr,
		       cfg->rx.active_queue.vaddr,
		       cfg->rx.num_buffers);

	dev->rx_queue.data = cfg->rx_data.vaddr;
	dev->rx_queue.notify_id = cfg->rx.id;
	carrels_rx_init(&dev->rx_queue);

	net_queue_init(&dev->tx_queue.handle,
		       cfg->tx.free_queue.vaddr,
		       cfg->tx.active_queue.vaddr,
		       cfg->tx.num_buffers);

	dev->tx_queue.data = cfg->tx_data.vaddr;
	dev->tx_queue.notify_id = cfg->tx.id;

	net_buffers_init(&dev->tx_queue.handle, 0);

	if (carrels_tx_init(&dev->tx_queue) != 0)
		return -EINVAL;

	return 0;
}

static int carrels_ndetdev_register(sddf_netdev_t *cdev)
{
	struct uk_alloc *a;
	int rc;

	a = uk_alloc_get_default();
	if (!a) {
		uk_pr_err("CARRELS: no default allocator for netdev registration\n");
		return -ENOMEM;
	}

	rc = uk_netdev_drv_register(&cdev->netdev, a, "uk_netdev_sddf");
	if (rc < 0) {
		uk_pr_err("CARRELS: uknetdev registration failed: %d\n", rc);
		return rc;
	}

	return 0;
}

static const struct uk_netdev_ops carrels_netdev_ops = {
	.probe = carrels_netdev_probe,
	.configure = carrels_netdev_configure,
	.rxq_configure = carrels_netdev_rxq_configure,
	.txq_configure = carrels_netdev_txq_configure,
	.start = carrels_netdev_start,

	.info_get = carrels_netdev_info_get,
	.rxq_info_get = carrels_netdev_rxq_info_get,
	.txq_info_get = carrels_netdev_txq_info_get,
	// @gt ?? set the values below are optional (and in fact most Nil),
	//        so similarly we disable mac/mtu/promisc set for sddf too.
	.hwaddr_get = carrels_netdev_hwaddr_get,
	.mtu_get = carrels_netdev_mtu_get,
	.promiscuous_get = carrels_netdev_promisc_get,
};

static int carrels_netdev_init(void)
{
	sddf_netdev_t *cdev = &carrels_dev;
	int rc;

	memset(cdev, 0, sizeof(*cdev));

	if (!net_config_check_magic(&net_config)) {
		uk_pr_err("CARRELS: invalid sDDF network configuration\n");
		return -EINVAL;
	}
	cdev->config = &net_config;

	rc = carrels_netdev_queues_init(cdev);
	if (rc < 0) {
		uk_pr_err("CARRELS: queue initialization failed: %d\n", rc);
		return rc;
	}

	/* Use the mac address provided by sddf netdev */
	memcpy(cdev->hwaddr.addr_bytes,
	       cdev->config->mac_addr.addr,
	       UK_NETDEV_HWADDR_LEN);

	/* Bind sddf network dev with Unikraft's driver framework */
	cdev->netdev.rx_one = carrels_netdev_rx_one;
	cdev->netdev.tx_one = carrels_netdev_tx_one;
	cdev->netdev.tx_buf_alloc = carrels_netdev_tx_buf_alloc;
	cdev->netdev.ops = &carrels_netdev_ops;

	// @gt ?? default=1500, which equals to what's provided by lwip
	//        in sddf other network drivers of unikraft have examples
	//        that is also 1500, so set it the way they are.
	// Ref: $(SDDF)/include/sddf/network/lib_sddf_lwip.h: 16
	cdev->mtu = UK_ETH_PAYLOAD_MAXLEN;
	// @gt :: sddf network driver support promiscuous mode by default,
	//        however, this promiscuous mode field is for the network
	//        stack inside the unikernel, which is a client to the sddf
	//        network subsystem that sees the client as an individual
	//        network stack using a fixed, virtualised mac address.
	//        So, just disable this promisc mode for within the unikernel
	// Ref: other unikraft network drivers (Xen, virtio-net)
	cdev->promisc = 0;

	rc = carrels_ndetdev_register(cdev);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

/* lwip stack uses UK_PRIO_LATEST and relies on netdev probing */
uk_lib_initcall_prio(carrels_netdev_init, 0, UK_PRIO_BEFORE(UK_PRIO_LATEST));
