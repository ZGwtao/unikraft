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

#include <carrels/netdev.h>

#define CARRELS_NETDEV_DRIVER_NAME	"sddf-net"
#define CARRELS_NETDEV_MTU		UK_ETH_PAYLOAD_MAXLEN
#define CARRELS_NETDEV_QUEUE_COUNT	1

UK_CTASSERT(MAC802_BYTES == UK_NETDEV_HWADDR_LEN);

extern net_client_config_t net_config;

struct uk_netdev_rx_queue {
	struct carrels_netdev *dev;
	uint16_t queue_id;
	uk_netdev_alloc_rxpkts alloc_rxpkts;
	void *alloc_rxpkts_argp;
};

struct uk_netdev_tx_queue {
	struct carrels_netdev *dev;
	uint16_t queue_id;
};

struct carrels_netdev {
	struct uk_netdev netdev;
	const net_client_config_t *config;

    net_queue_handle_t rx_handle;
	net_queue_handle_t tx_handle;
	net_queue_handle_t *rxq;
	net_queue_handle_t *txq;

	struct uk_netdev_rx_queue rx_queue;
	struct uk_netdev_tx_queue tx_queue;

	struct uk_hwaddr hwaddr;

	uint16_t mtu;
	unsigned int configured;
	unsigned int running;
	int id;
};

static inline struct carrels_netdev *
to_carrels_netdev(struct uk_netdev *dev)
{
	UK_ASSERT(dev);

	return __containerof(dev, struct carrels_netdev, netdev);
}

static int carrels_netdev_probe(struct uk_netdev *dev)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	UK_ASSERT(cdev->config);
	UK_ASSERT(cdev->rxq);
	UK_ASSERT(cdev->txq);

	uk_pr_info("CARRELS: probing sDDF network device\n");

	return 0;
}

static void carrels_netdev_info_get(struct uk_netdev *dev,
				    struct uk_netdev_info *info)
{
	UK_ASSERT(dev);
	UK_ASSERT(info);

	memset(info, 0, sizeof(*info));

	info->max_rx_queues = CARRELS_NETDEV_QUEUE_COUNT;
	info->max_tx_queues = CARRELS_NETDEV_QUEUE_COUNT;
	info->in_queue_pairs = 1;
	info->max_mtu = CARRELS_NETDEV_MTU;
	info->nb_encap_rx = 0;
	info->nb_encap_tx = 0;
	info->ioalign = 16;
	info->features = 0;
}

static int carrels_netdev_rxq_info_get(struct uk_netdev *dev,
				       uint16_t queue_id,
				       struct uk_netdev_queue_info *info)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	if (!info || queue_id != 0)
		return -EINVAL;

	memset(info, 0, sizeof(*info));

	info->nb_min = 1;
	info->nb_max = (uint16_t)cdev->rxq->capacity;
	info->nb_align = 1;
	info->nb_is_power_of_two = 0;

	return 0;
}

static int carrels_netdev_txq_info_get(struct uk_netdev *dev,
				       uint16_t queue_id,
				       struct uk_netdev_queue_info *info)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	if (!info || queue_id != 0)
		return -EINVAL;

	memset(info, 0, sizeof(*info));

	info->nb_min = 1;
	info->nb_max = (uint16_t)cdev->txq->capacity;
	info->nb_align = 1;
	info->nb_is_power_of_two = 0;

	return 0;
}

static int carrels_netdev_configure(struct uk_netdev *dev,
				    const struct uk_netdev_conf *conf)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	if (!conf)
		return -EINVAL;

	if (conf->nb_rx_queues != CARRELS_NETDEV_QUEUE_COUNT ||
	    conf->nb_tx_queues != CARRELS_NETDEV_QUEUE_COUNT)
		return -EINVAL;

	if (conf->lro)
		return -ENOTSUP;

	cdev->configured = 1;

	uk_pr_info("CARRELS: configured sDDF netdev with one RX and one TX queue\n");

	return 0;
}

static struct uk_netdev_rx_queue *
carrels_netdev_rxq_configure(struct uk_netdev *dev,
			     uint16_t queue_id,
			     uint16_t nb_desc,
			     struct uk_netdev_rxqueue_conf *conf)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	if (!conf || queue_id != 0)
		return ERR2PTR(-EINVAL);

	if (!conf->alloc_rxpkts)
		return ERR2PTR(-EINVAL);

	if (nb_desc == 0)
		nb_desc = (uint16_t)cdev->rxq->capacity;

	if (nb_desc > cdev->rxq->capacity)
		return ERR2PTR(-EINVAL);

	cdev->rx_queue.dev = cdev;
	cdev->rx_queue.queue_id = queue_id;
	cdev->rx_queue.alloc_rxpkts = conf->alloc_rxpkts;
	cdev->rx_queue.alloc_rxpkts_argp = conf->alloc_rxpkts_argp;

	return &cdev->rx_queue;
}

static struct uk_netdev_tx_queue *
carrels_netdev_txq_configure(struct uk_netdev *dev,
			     uint16_t queue_id,
			     uint16_t nb_desc,
			     struct uk_netdev_txqueue_conf *conf)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	if (!conf || queue_id != 0)
		return ERR2PTR(-EINVAL);

	if (nb_desc == 0)
		nb_desc = (uint16_t)cdev->txq->capacity;

	if (nb_desc > cdev->txq->capacity)
		return ERR2PTR(-EINVAL);

	cdev->tx_queue.dev = cdev;
	cdev->tx_queue.queue_id = queue_id;

	return (struct uk_netdev_tx_queue *)&cdev->tx_queue;
}

static int carrels_netdev_start(struct uk_netdev *dev)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	if (!cdev->configured)
		return -EINVAL;

	cdev->running = 1;

	uk_pr_info("CARRELS: sDDF netdev started\n");

	return 0;
}

static const struct uk_hwaddr *
carrels_netdev_hwaddr_get(struct uk_netdev *dev)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	return &cdev->hwaddr;
}

static uint16_t carrels_netdev_mtu_get(struct uk_netdev *dev)
{
	struct carrels_netdev *cdev = to_carrels_netdev(dev);

	return cdev->mtu;
}

static unsigned int carrels_netdev_promiscuous_get(struct uk_netdev *dev)
{
	(void)dev;

	return 0;
}

static size_t carrels_netbuf_len(const struct uk_netbuf *pkt)
{
	const struct uk_netbuf *seg;
	size_t total = 0;

	for (seg = pkt; seg != NULL; seg = seg->next)
		total += seg->len;

	return total;
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

static int
carrels_netdev_rx_one(struct uk_netdev *dev,
		      struct uk_netdev_rx_queue *queue,
		      struct uk_netbuf **pkt)
{
	struct carrels_netdev *cdev;
	struct uk_netbuf *rx_pkts[1] = { NULL };
	struct uk_netbuf *nb;
	net_buff_desc_t desc;
	uint8_t *src;
	size_t frame_len;
	uint16_t allocated;
	int status;
	int rc;

	if (unlikely(!dev || !queue || !pkt))
		return 0;

	*pkt = NULL;
	cdev = to_carrels_netdev(dev);

	if (unlikely(queue != &cdev->rx_queue))
		return 0;

	if (unlikely(!cdev->running))
		return 0;

	if (net_queue_empty_active(cdev->rxq))
		return 0;

	rc = net_dequeue_active(cdev->rxq, &desc);
	if (unlikely(rc != 0))
		return 0;

	frame_len = desc.len;

	if (unlikely(frame_len < 14 || frame_len > NET_BUFFER_SIZE)) {
		uk_pr_info("CARRELS RX: invalid frame length %u\n",
			    (unsigned int)frame_len);
		goto recycle;
	}

	src = (uint8_t *)cdev->config->rx_data.vaddr +
	      desc.io_or_offset;

	if (unlikely(!queue->alloc_rxpkts)) {
		uk_pr_info("CARRELS RX: allocator callback is NULL\n");
		goto recycle;
	}

	allocated = queue->alloc_rxpkts(
		queue->alloc_rxpkts_argp,
		rx_pkts,
		1
	);

	if (unlikely(allocated != 1 || !rx_pkts[0])) {
		uk_pr_info(
			"CARRELS RX: netbuf allocation failed "
			"allocated=%u alloc=%p arg=%p\n",
			allocated,
			queue->alloc_rxpkts,
			queue->alloc_rxpkts_argp
		);
		goto recycle;
	}

	nb = rx_pkts[0];

	if (unlikely(frame_len > nb->len)) {
		uk_pr_info(
			"CARRELS RX: frame %u exceeds capacity %u\n",
			(unsigned int)frame_len,
			(unsigned int)nb->len
		);
		uk_netbuf_free(nb);
		goto recycle;
	}

	memcpy(nb->data, src, frame_len);
	nb->len = frame_len;

	*pkt = nb;
recycle:
	desc.len = 0;

	rc = net_enqueue_free(cdev->rxq, desc);
	if (unlikely(rc != 0))
		UK_CRASH("CARRELS: failed to recycle RX descriptor");

	if (!*pkt)
		return 0;

	status = UK_NETDEV_STATUS_SUCCESS;

	if (!net_queue_empty_active(cdev->rxq))
		status |= UK_NETDEV_STATUS_MORE;

	return status;
}

static int carrels_netdev_tx_one(struct uk_netdev *dev,
				 struct uk_netdev_tx_queue *queue,
				 struct uk_netbuf *pkt)
{
	struct carrels_netdev *cdev;
	net_buff_desc_t desc;
	uint8_t *dst;
	size_t pkt_len;
	bool notify;
	int status;
	int rc;

	if (unlikely(!dev || !queue || !pkt))
		return 0;

	cdev = to_carrels_netdev(dev);

	if (unlikely(queue != &cdev->tx_queue))
		return 0;

	if (unlikely(!cdev->running))
		return 0;

	pkt_len = carrels_netbuf_len(pkt);

	if (unlikely(pkt_len == 0 || pkt_len > NET_BUFFER_SIZE)) {
		uk_pr_err("CARRELS: invalid TX packet length %zu\n", pkt_len);
		return 0;
	}

	rc = net_dequeue_free(cdev->txq, &desc);
	if (rc != 0)
		return 0;

	dst = (uint8_t *)cdev->config->tx_data.vaddr +
	      desc.io_or_offset;

	carrels_netbuf_copy(dst, pkt);

    desc.len = (uint16_t)pkt_len;

    rc = net_enqueue_active(cdev->txq, desc);
    if (unlikely(rc != 0)) {
        rc = net_enqueue_free(cdev->txq, desc);
        UK_ASSERT(rc == 0);
        return 0;
    }

    if (net_require_signal_active(cdev->txq)) {
        net_cancel_signal_active(cdev->txq);
        microkit_notify(cdev->config->tx.id);

        uk_pr_info("CARRELS TX: notified channel=%u\n",
                (unsigned int)cdev->config->tx.id);
    } else {
        uk_pr_info("CARRELS TX: notification not requested\n");
    }

	uk_netbuf_free(pkt);

	status = UK_NETDEV_STATUS_SUCCESS;

	if (!net_queue_empty_free(cdev->txq) &&
	    !net_queue_full_active(cdev->txq))
		status |= UK_NETDEV_STATUS_MORE;

	return status;
}

static int carrels_netdev_queues_init(struct carrels_netdev *dev)
{
	const net_client_config_t *cfg = dev->config;

	if (!cfg)
		return -EINVAL;

	net_queue_init(&dev->rx_handle,
		       cfg->rx.free_queue.vaddr,
		       cfg->rx.active_queue.vaddr,
		       cfg->rx.num_buffers);

	net_queue_init(&dev->tx_handle,
		       cfg->tx.free_queue.vaddr,
		       cfg->tx.active_queue.vaddr,
		       cfg->tx.num_buffers);

	net_buffers_init(&dev->tx_handle, 0);

	dev->rxq = &dev->rx_handle;
	dev->txq = &dev->tx_handle;

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

	.hwaddr_get = carrels_netdev_hwaddr_get,
	.mtu_get = carrels_netdev_mtu_get,
	.promiscuous_get = carrels_netdev_promiscuous_get,
};

static struct carrels_netdev carrels_dev = {
	.config = &net_config,
};

static int carrels_netdev_init(void)
{
	struct uk_alloc *a;
	struct carrels_netdev *cdev = &carrels_dev;
	int rc;

	memset(cdev, 0, sizeof(*cdev));

	cdev->config = &net_config;
	cdev->mtu = CARRELS_NETDEV_MTU;

	rc = carrels_netdev_queues_init(cdev);
	if (rc < 0) {
		uk_pr_err("CARRELS: queue initialization failed: %d\n", rc);
		return rc;
	}

	memcpy(cdev->hwaddr.addr_bytes,
	       cdev->config->mac_addr.addr,
	       UK_NETDEV_HWADDR_LEN);

	cdev->netdev.rx_one = carrels_netdev_rx_one;
	cdev->netdev.tx_one = carrels_netdev_tx_one;
	cdev->netdev.ops = &carrels_netdev_ops;

	a = uk_alloc_get_default();
	if (!a) {
		uk_pr_err("CARRELS: no default allocator for netdev registration\n");
		return -ENOMEM;
	}

	rc = uk_netdev_drv_register(&cdev->netdev,
				    a,
				    CARRELS_NETDEV_DRIVER_NAME);
	if (rc < 0) {
		uk_pr_err("CARRELS: uknetdev registration failed: %d\n", rc);
		return rc;
	}

	cdev->id = rc;

	uk_pr_info(
		"CARRELS: registered sDDF netdev id=%d "
		"MAC=%02x:%02x:%02x:%02x:%02x:%02x "
		"RX buffers=%u TX buffers=%u\n",
		cdev->id,
		cdev->hwaddr.addr_bytes[0],
		cdev->hwaddr.addr_bytes[1],
		cdev->hwaddr.addr_bytes[2],
		cdev->hwaddr.addr_bytes[3],
		cdev->hwaddr.addr_bytes[4],
		cdev->hwaddr.addr_bytes[5],
		(unsigned int)cdev->rxq->capacity,
		(unsigned int)cdev->txq->capacity);

	return 0;
}

uk_lib_initcall_prio(carrels_netdev_init, 0, UK_PRIO_LATEST);