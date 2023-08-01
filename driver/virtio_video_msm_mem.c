/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/kthread.h>
#include <linux/kernel.h>
#include "virtio_video_msm_hab.h"
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/habmm.h>
#include "virtio_video_msm_debug.h"
#include <media/v4l2-common.h>
#include "virtio_video.h"
#include "virtio_video_msm_mem.h"

#define MAX_EXPORT_RETRY 5
#define MAX_NUM_EXPORT_CACHE_ENTRY 64


static struct buf_export_entry* get_entry_from_fd(struct list_head *fifo,
						  uint64_t fd, uint32_t size,
						  enum virtio_video_queue_type buf_type)
{
	struct buf_export_entry *entry = NULL, *found = NULL;
	struct dma_buf *dmabuf = NULL;
	uint64_t inode = 0;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("%s: dma_buf_get error fd=0x%x", __func__, fd);
		goto exit;
	}

	inode = (uint64_t)dmabuf->file->f_inode;
	list_for_each_entry(entry, fifo, list) {
		if ((entry->inode == inode) && (entry->buf_type == buf_type) && (entry->size == size)) {
			found = entry;
			pr_info("%s: fd=%#x, inode=%#x, export_id=%d, buffer_type=%d, size=%d",
				 __func__, fd, inode, entry->buffer_id, buf_type, size);
			break;
		}
	}
	dma_buf_put(dmabuf);

exit:
	return found;
}

static struct buf_export_entry* alloc_one_export_entry(struct buf_export_cache* cache,
						       uint64_t fd, uint32_t size,
						       enum virtio_video_queue_type buf_type,
						       uint32_t export_id)
{
	struct buf_export_entry* entry = NULL;
	struct dma_buf *dmabuf = NULL;
	uint64_t inode = 0;

	if (cache->used_count >= MAX_NUM_EXPORT_CACHE_ENTRY) {
		pr_err("%s: export cache is full", __func__, fd);
		goto exit;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("%s: dma_buf_get error fd=0x%x", __func__, fd);
		goto exit;
	}

	inode = (uint64_t)dmabuf->file->f_inode;
	entry = kmem_cache_alloc(cache->exports, GFP_KERNEL);

	if (!entry) {
		entry = ERR_PTR(-ENOMEM);
	} else {
		memset(entry, 0, sizeof(*entry));

		entry->inode = inode;
		entry->buffer_id = export_id;
		entry->size = size;
		entry->buf_type = buf_type;

		list_add_tail(&entry->list, &cache->export_fifo);
		cache->used_count++;
		pr_info("%s: fd=%#x, inode=%#x, export_id=%d, size =%d, buf_type=%d, cache_size=%d",
			__func__, fd, inode, export_id, size, buf_type, cache->used_count);
	}

	dma_buf_put(dmabuf);

exit:
	return entry;
}

uint32_t msm_buf_get_export_id(struct virtio_video_stream* stream,
			       uint32_t habmmhandle, uint64_t fd, uint32_t size,
			       enum virtio_video_queue_type buf_type, bool export_as_fd)
{
	int ret = 0;
	uint32_t export_id = 0;
	struct v4l2_device *vd = stream->video_dev->v4l2_dev;
	const uint32_t export_flag = HABMM_EXPIMP_FLAGS_FD;
	int i = 0;
	struct buf_export_entry *find_entry = NULL;
	struct buf_export_entry *entry = NULL;

	if (!size) {
		v4l2_err(vd, "%s: size 0. skip export", __func__);
		goto exit;
	}

	find_entry = get_entry_from_fd(&stream->buf_cache.export_fifo, fd, size, buf_type);
	if (find_entry) {
		export_id = find_entry->buffer_id;
	} else {
		for (i = 0; i < MAX_EXPORT_RETRY && ret != -ENOMEM; i++) {
			ret = habmm_export(habmmhandle, (void*)fd, size, &export_id,
					   export_flag);
			if (ret) {
				v4l2_err(vd, "%s: export failed. retry %d rc %d",
					 __func__, i, ret);
			} else {
				v4l2_info(vd, "%s: export ok, type %d fd %d export_id %d sz %d retry %d",
					  __func__, buf_type, fd, export_id, size, i);
				break;
			}
		}

		if (unlikely(ret) || unlikely(!export_id)) {
			v4l2_err(vd, "%s: export failed. buf type %d fd %d sz %d", __func__,
				 buf_type, fd, size);
			export_id = 0;
			goto exit;
		}

		if (buf_type == OUTPUT_MPLANE || buf_type == OUTPUT_META_PLANE) {
			entry = alloc_one_export_entry(&stream->buf_cache, fd, size,
						       buf_type, export_id);
			if (unlikely(IS_ERR(entry))) {
				v4l2_err(vd, "alloc entry failed");

				if (habmm_unexport(habmmhandle, export_id, 0))
					v4l2_err(vd, "failed to unexport id %d", export_id);

				export_id = 0;
			} else {
				v4l2_info(vd, "%s: alloc entry. buf type %d fd %d export_id %d sz %d",
					  __func__, buf_type, fd, export_id, size);
			}
		}
	}

exit:
	return export_id;
}

int msm_buf_put_export_id(struct virtio_video_stream* stream, uint32_t export_id,
			  enum virtio_video_event_type event_type, uint32_t flags)
{
	int ret = 0;
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	uint32_t habmmhandle = vvd->commandq.vq->habmm_handle;

	v4l2_info(&vvd->v4l2_dev, "%s: event_type=%#x, export_id=%d, flags=%#x\n",
		  __func__, event_type, export_id, flags);

	if (event_type == VIRTIO_VIDEO_EVENT_EBD) {
		ret = habmm_unexport(habmmhandle, export_id, 0);
		if (ret) {
			v4l2_err(&vvd->v4l2_dev, "%s: failed to unexport id %d, ret=%d",
				 __func__, export_id, ret);
		} else {
			v4l2_info(&vvd->v4l2_dev, "%s: unexport ok, export_id=%d",
				  __func__, export_id);
		}
	}

	return ret;
}

int msm_export_cache_init(struct buf_export_cache* cache)
{
	int ret = 0;

	cache->exports = kmem_cache_create("virtio-video-exp-cache",
					   sizeof(struct buf_export_entry), 0, 0, NULL);
	if (!cache->exports) {
		ret = -ENOMEM;
		goto exit;
	}

	INIT_LIST_HEAD(&cache->export_fifo);
	cache->used_count = 0;

exit:
	return ret;
}

void msm_export_cache_destroy(struct buf_export_cache* cache)
{
	struct buf_export_entry* entry = NULL, *temp = NULL;

	list_for_each_entry_safe(entry, temp, &cache->export_fifo, list) {
		list_del(&entry->list);
		kmem_cache_free(cache->exports, entry);
	}
	kmem_cache_destroy(cache->exports);
	cache->used_count = 0;
}
