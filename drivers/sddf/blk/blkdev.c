/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <carrels/events.h>
#include <microkit.h>
#include <sddf/blk/config.h>
#include <sddf/blk/queue.h>
#include <sddf/blk/storage_info.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/blkdev.h>
#include <uk/blkdev_driver.h>
#include <uk/errptr.h>
#include <uk/init.h>
#include <uk/print.h>

#define SDDF_BLK_DRIVER_NAME "sddf-blk"
#define SDDF_BLK_QUEUE_COUNT 1

extern blk_client_config_t blk_config;

struct sddf_blk_request {
	struct uk_blkreq *req;
	uint32_t data_block;
	uint16_t data_count;
	bool used;
};

struct sddf_blk_device;

struct uk_blkdev_queue {
	struct sddf_blk_device *dev;
	uint16_t nb_desc;
	bool intr_enabled;
};

struct sddf_blk_device {
	struct uk_blkdev blkdev;
	const blk_client_config_t *config;
	blk_storage_info_t *storage_info;
	blk_queue_handle_t handle;
	struct uk_blkdev_queue queue;
	struct sddf_blk_request *requests;
	uint8_t *data_used;
	uint32_t data_blocks;
	uint16_t outstanding;
	bool configured;
	bool running;
	int id;
};

static struct sddf_blk_device sddf_blkdev;

static inline struct sddf_blk_device *to_sddf_blkdev(struct uk_blkdev *dev)
{
	return __containerof(dev, struct sddf_blk_device, blkdev);
}

static int data_alloc(struct sddf_blk_device *dev, uint16_t count,
		      uint32_t *first)
{
	uint32_t i, j;

	if (!count) {
		*first = 0;
		return 0;
	}

	for (i = 0; i + count <= dev->data_blocks; i++) {
		for (j = 0; j < count && !dev->data_used[i + j]; j++)
			;
		if (j == count) {
			memset(&dev->data_used[i], 1, count);
			*first = i;
			return 0;
		}
		i += j;
	}
	return -ENOSPC;
}

static void data_free(struct sddf_blk_device *dev, uint32_t first,
		      uint16_t count)
{
	UK_ASSERT(first + count <= dev->data_blocks);
	memset(&dev->data_used[first], 0, count);
}

static int request_slot_alloc(struct sddf_blk_device *dev)
{
	uint32_t i;

	for (i = 0; i < dev->handle.capacity; i++)
		if (!dev->requests[i].used) {
			dev->requests[i].used = true;
			return (int)i;
		}
	return -ENOSPC;
}

static int response_error(blk_resp_status_t status)
{
	switch (status) {
	case BLK_RESP_OK: return 0;
	case BLK_RESP_ERR_INVALID_PARAM: return -EINVAL;
	case BLK_RESP_ERR_NO_DEVICE: return -ENODEV;
	case BLK_RESP_ERR_IO: return -EIO;
	default: return -EIO;
	}
}

static void sddf_blk_get_info(struct uk_blkdev *dev __unused,
			      struct uk_blkdev_info *info)
{
	info->max_queues = SDDF_BLK_QUEUE_COUNT;
}

static int sddf_blk_configure(struct uk_blkdev *dev,
			      const struct uk_blkdev_conf *conf)
{
	struct sddf_blk_device *bdev = to_sddf_blkdev(dev);

	if (!conf || conf->nb_queues != SDDF_BLK_QUEUE_COUNT)
		return -EINVAL;
	bdev->configured = true;
	return 0;
}

static int sddf_blk_queue_get_info(struct uk_blkdev *dev, uint16_t queue_id,
				   struct uk_blkdev_queue_info *info)
{
	struct sddf_blk_device *bdev = to_sddf_blkdev(dev);

	if (!info || queue_id)
		return -EINVAL;
	info->nb_min = 1;
	info->nb_max = (uint16_t)bdev->handle.capacity;
	info->nb_align = 1;
	info->nb_is_power_of_two = 0;
	return 0;
}

static struct uk_blkdev_queue *
sddf_blk_queue_configure(struct uk_blkdev *dev, uint16_t queue_id,
			 uint16_t nb_desc,
			 const struct uk_blkdev_queue_conf *conf)
{
	struct sddf_blk_device *bdev = to_sddf_blkdev(dev);

	if (!conf || queue_id)
		return ERR2PTR(-EINVAL);
	if (!nb_desc)
		nb_desc = (uint16_t)bdev->handle.capacity;
	if (nb_desc > bdev->handle.capacity)
		return ERR2PTR(-EINVAL);
	bdev->queue.dev = bdev;
	bdev->queue.nb_desc = nb_desc;
	return &bdev->queue;
}

static int sddf_blk_start(struct uk_blkdev *dev)
{
	struct sddf_blk_device *bdev = to_sddf_blkdev(dev);

	if (!bdev->configured || !blk_storage_is_ready(bdev->storage_info))
		return -ENODEV;
	bdev->running = true;
	return 0;
}

static int sddf_blk_stop(struct uk_blkdev *dev)
{
	struct sddf_blk_device *bdev = to_sddf_blkdev(dev);

	if (bdev->outstanding)
		return -EBUSY;
	bdev->running = false;
	return 0;
}

static int sddf_blk_intr_enable(struct uk_blkdev *dev __unused,
				struct uk_blkdev_queue *queue)
{
	queue->intr_enabled = true;
	return blk_queue_empty_resp(&queue->dev->handle) ? 0 : 1;
}

static int sddf_blk_intr_disable(struct uk_blkdev *dev __unused,
				 struct uk_blkdev_queue *queue)
{
	queue->intr_enabled = false;
	return 0;
}

static int sddf_blk_queue_unconfigure(struct uk_blkdev *dev __unused,
				      struct uk_blkdev_queue *queue)
{
	if (queue->dev->outstanding)
		return -EBUSY;
	queue->nb_desc = 0;
	queue->intr_enabled = false;
	return 0;
}

static int sddf_blk_unconfigure(struct uk_blkdev *dev)
{
	to_sddf_blkdev(dev)->configured = false;
	return 0;
}

static int sddf_blk_submit_one(struct uk_blkdev *dev,
			       struct uk_blkdev_queue *queue,
			       struct uk_blkreq *req)
{
	struct sddf_blk_device *bdev = to_sddf_blkdev(dev);
	struct sddf_blk_request *pending;
	blk_req_code_t code;
	uint32_t data_block = 0;
	uint16_t count;
	int id, rc, status;

	if (!req || queue != &bdev->queue || !bdev->running)
		return -EINVAL;
	if (bdev->outstanding >= queue->nb_desc || blk_queue_full_req(&bdev->handle))
		return -ENOSPC;
	if (req->operation == UK_BLKREQ_FFLUSH) {
		code = BLK_REQ_FLUSH;
		count = 0;
	} else {
		if (req->operation != UK_BLKREQ_READ &&
		    req->operation != UK_BLKREQ_WRITE)
			return -EINVAL;
		if (!req->aio_buf || !req->nb_sectors ||
		    req->nb_sectors > bdev->blkdev.capabilities.max_sectors_per_req ||
		    req->start_sector >= bdev->blkdev.capabilities.sectors ||
		    req->nb_sectors > bdev->blkdev.capabilities.sectors -
				      req->start_sector)
			return -EINVAL;
		if (req->operation == UK_BLKREQ_WRITE &&
		    bdev->blkdev.capabilities.mode == O_RDONLY)
			return -EROFS;
		count = (uint16_t)req->nb_sectors;
		code = req->operation == UK_BLKREQ_READ ? BLK_REQ_READ : BLK_REQ_WRITE;
		rc = data_alloc(bdev, count, &data_block);
		if (rc)
			return rc;
		if (code == BLK_REQ_WRITE)
			memcpy((uint8_t *)bdev->config->data.vaddr +
			       (size_t)data_block * BLK_TRANSFER_SIZE,
			       req->aio_buf, (size_t)count * BLK_TRANSFER_SIZE);
	}

	id = request_slot_alloc(bdev);
	if (id < 0) {
		data_free(bdev, data_block, count);
		return id;
	}
	pending = &bdev->requests[id];
	pending->req = req;
	pending->data_block = data_block;
	pending->data_count = count;

	rc = blk_enqueue_req(&bdev->handle, code,
			     (uintptr_t)data_block * BLK_TRANSFER_SIZE,
			     req->start_sector, count, (uint32_t)id);
	if (rc) {
		pending->used = false;
		data_free(bdev, data_block, count);
		return -ENOSPC;
	}
	bdev->outstanding++;
	microkit_notify(bdev->config->virt.id);

	status = UK_BLKDEV_STATUS_SUCCESS;
	if (bdev->outstanding < queue->nb_desc &&
	    !blk_queue_full_req(&bdev->handle))
		status |= UK_BLKDEV_STATUS_MORE;
	return status;
}

static int sddf_blk_finish_reqs(struct uk_blkdev *dev,
				struct uk_blkdev_queue *queue)
{
	struct sddf_blk_device *bdev = to_sddf_blkdev(dev);
	blk_resp_status_t response;
	uint16_t success_count;
	uint32_t id;
	int rc;

	if (queue != &bdev->queue)
		return -EINVAL;
	while (!blk_dequeue_resp(&bdev->handle, &response, &success_count, &id)) {
		struct sddf_blk_request *pending;
		struct uk_blkreq *req;

		if (id >= bdev->handle.capacity || !bdev->requests[id].used) {
			uk_pr_err("sDDF blk: invalid response id %u\n", id);
			continue;
		}
		pending = &bdev->requests[id];
		req = pending->req;
		rc = response_error(response);
		if (!rc && pending->data_count &&
		    success_count != pending->data_count)
			rc = -EIO;
		if (!rc && req->operation == UK_BLKREQ_READ)
			memcpy(req->aio_buf,
			       (uint8_t *)bdev->config->data.vaddr +
			       (size_t)pending->data_block * BLK_TRANSFER_SIZE,
			       (size_t)pending->data_count * BLK_TRANSFER_SIZE);
		data_free(bdev, pending->data_block, pending->data_count);
		pending->used = false;
		pending->req = NULL;
		bdev->outstanding--;
		req->result = rc;
		uk_blkreq_finished(req);
		if (req->cb)
			req->cb(req, req->cb_cookie);
	}
	return 0;
}

static void sddf_blk_event(microkit_channel ch, void *arg)
{
	struct sddf_blk_device *bdev = arg;

	UK_ASSERT(ch == bdev->config->virt.id);
	if (bdev->running && bdev->queue.intr_enabled)
		uk_blkdev_drv_queue_event(&bdev->blkdev, 0);
}

static const struct uk_blkdev_ops sddf_blk_ops = {
	.get_info = sddf_blk_get_info,
	.dev_configure = sddf_blk_configure,
	.queue_get_info = sddf_blk_queue_get_info,
	.queue_configure = sddf_blk_queue_configure,
	.dev_start = sddf_blk_start,
	.dev_stop = sddf_blk_stop,
	.queue_intr_enable = sddf_blk_intr_enable,
	.queue_intr_disable = sddf_blk_intr_disable,
	.queue_unconfigure = sddf_blk_queue_unconfigure,
	.dev_unconfigure = sddf_blk_unconfigure,
};

static int sddf_blk_init(struct uk_init_ctx *ctx __unused)
{
	struct sddf_blk_device *bdev = &sddf_blkdev;
	struct uk_alloc *a = uk_alloc_get_default();
	uint64_t max_blocks;
	int rc;

	if (!a)
		return -ENOMEM;
	if (!blk_config_check_magic(&blk_config))
		return -EINVAL;
	memset(bdev, 0, sizeof(*bdev));
	bdev->config = &blk_config;
	bdev->storage_info = blk_config.virt.storage_info.vaddr;
	blk_queue_init(&bdev->handle, blk_config.virt.req_queue.vaddr,
		       blk_config.virt.resp_queue.vaddr,
		       blk_config.virt.num_buffers);
	if (!bdev->handle.capacity || !blk_config.data.size)
		return -EINVAL;
	bdev->data_blocks = (uint32_t)(blk_config.data.size / BLK_TRANSFER_SIZE);
	if (!bdev->data_blocks)
		return -EINVAL;
	bdev->requests = uk_calloc(a, bdev->handle.capacity,
				   sizeof(*bdev->requests));
	bdev->data_used = uk_calloc(a, bdev->data_blocks,
				    sizeof(*bdev->data_used));
	if (!bdev->requests || !bdev->data_used) {
		uk_free(a, bdev->requests);
		uk_free(a, bdev->data_used);
		return -ENOMEM;
	}
	if (!blk_storage_is_ready(bdev->storage_info)) {
		rc = -ENODEV;
		goto err_free;
	}
	max_blocks = bdev->data_blocks < UINT16_MAX ? bdev->data_blocks : UINT16_MAX;
	bdev->blkdev.capabilities.sectors = bdev->storage_info->capacity;
	bdev->blkdev.capabilities.ssize = BLK_TRANSFER_SIZE;
	bdev->blkdev.capabilities.mode = bdev->storage_info->read_only ? O_RDONLY : O_RDWR;
	bdev->blkdev.capabilities.max_sectors_per_req = max_blocks;
	bdev->blkdev.capabilities.ioalign = BLK_TRANSFER_SIZE;
	bdev->blkdev.submit_one = sddf_blk_submit_one;
	bdev->blkdev.finish_reqs = sddf_blk_finish_reqs;
	bdev->blkdev.dev_ops = &sddf_blk_ops;
	rc = carrels_event_register(blk_config.virt.id, sddf_blk_event, bdev);
	if (rc)
		goto err_free;
	rc = uk_blkdev_drv_register(&bdev->blkdev, a, SDDF_BLK_DRIVER_NAME);
	if (rc < 0) {
		carrels_event_unregister(blk_config.virt.id, sddf_blk_event, bdev);
		goto err_free;
	}
	bdev->id = rc;
	uk_pr_info("sDDF blk: registered blkdev%d, %llu x %u bytes, queue %u\n",
		   bdev->id, (unsigned long long)bdev->storage_info->capacity,
		   BLK_TRANSFER_SIZE, bdev->handle.capacity);
	return 0;

err_free:
	uk_free(a, bdev->requests);
	uk_free(a, bdev->data_used);
	return rc;
}

uk_lib_initcall_prio(sddf_blk_init, 0, UK_PRIO_BEFORE(UK_PRIO_LATEST));
