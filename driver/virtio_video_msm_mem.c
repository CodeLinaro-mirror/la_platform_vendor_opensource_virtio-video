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

#define V4L2FE_ALIGN(x, to_align) ((((unsigned) x) + (to_align - 1)) & ~(to_align - 1))
#define VIRTIO_VIDEO_BUF_FLAG_READONLY      0x20000000
#define NO_CACHE

#ifndef NO_CACHE
static struct buf_export_entry* get_entry_from_export_id(struct list_head* fifo,
							 uint32_t id)
{
	struct buf_export_entry* entry = NULL;

	list_for_each_entry(entry, fifo, list) {
		if (entry->buffer_id == id)
			break;
	}

	return entry;
}

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
			pr_info("%s: fd=%#x, inode=%#x, export_id=%x, buffer_type=%d, size=%d",
				 __func__, fd, inode, entry->buffer_id, buf_type, size);
			break;
		}
	}
	dma_buf_put(dmabuf);

exit:
	return found;
}

static uint32_t free_oldest_export_entry(struct buf_export_cache* cache)
{
	uint32_t buffer_id = 0;
	struct buf_export_entry* entry = NULL, *temp = NULL;

	list_for_each_entry_safe(entry, temp, &cache->export_fifo, list) {
		if (!entry->is_export) {
			buffer_id = entry->buffer_id;
			list_del(&entry->list);
			kmem_cache_free(cache->exports, entry);
			cache->export_avail++;
			break;
		}
	}

	return buffer_id;
}

static struct buf_export_entry* alloc_one_export_entry(struct buf_export_cache* cache,
						       uint64_t fd, uint32_t size,
						       enum virtio_video_queue_type buf_type,
						       uint32_t export_id)
{
	struct buf_export_entry* entry = NULL;
	struct dma_buf *dmabuf = NULL;
	uint64_t inode = 0;

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
		entry->is_export = true;

		list_add_tail(&entry->list, &cache->export_fifo);
		cache->export_avail--;
		pr_info("%s: fd=%#x, inode=%#x, export_id=%x, size =%d, buf_type=%d, export_avail=%d",
			__func__, fd, inode, export_id, size, buf_type, cache->export_avail);
	}

	dma_buf_put(dmabuf);

exit:
	return entry;
}
#endif

uint32_t msm_buf_get_export_id(struct virtio_video_stream* stream,
			       uint32_t habmmhandle, uint64_t fd, uint32_t size,
			       enum virtio_video_queue_type buf_type, bool export_as_fd)
{
	int ret = 0;
	uint32_t export_id = 0;
	struct v4l2_device *vd = stream->video_dev->v4l2_dev;
	const uint32_t export_flag = HABMM_EXPIMP_FLAGS_FD;
	int i = 0;
	uint32_t algn_size = V4L2FE_ALIGN(size, 4096);
#ifndef NO_CACHE
	uint32_t buf_id = 0;
	struct buf_export_entry *find_entry = NULL;
	struct buf_export_entry *entry = NULL;
#endif

	if (!size) {
		v4l2_err(vd, "%s: size 0. skip export", __func__);
		goto exit;
	}

#ifndef NO_CACHE
	find_entry = get_entry_from_fd(&stream->buf_cache.export_fifo, fd, algn_size, buf_type);
	if (find_entry) {
		export_id = find_entry->buffer_id;
	} else {
#endif
		for (i = 0; i < MAX_EXPORT_RETRY && ret != -ENOMEM; i++) {
			ret = habmm_export(habmmhandle, (void*)fd, algn_size, &export_id,
					   export_flag);
			if (ret) {
				v4l2_err(vd, "%s: export failed. retry %d rc %d",
					 __func__, i, ret);
			} else {
				v4l2_info(vd, "%s: export ok, type %d fd %d export_id %d sz %d retry %d",
					  __func__, buf_type, fd, export_id, algn_size, i);
				break;
			}
		}

		if (unlikely(!export_id)) {
			v4l2_err(vd, "%s: export failed. buf type %d fd %d sz %d", __func__,
				 buf_type, fd, algn_size);
				goto exit;
		}
#ifndef NO_CACHE
		if (stream->buf_cache.export_avail <= 0) {
			v4l2_info(vd, "buf_cache overflow, remove the oldest unused one");
			buf_id = free_oldest_export_entry(&stream->buf_cache);

			if (habmm_unexport(habmmhandle, buf_id, 0)) {
				v4l2_err(vd, "failed to unexport buffer id %d", buf_id);
			}
		}

		entry = alloc_one_export_entry(&stream->buf_cache, fd, algn_size,
					       buf_type, export_id);
		if (unlikely(IS_ERR(entry))) {
			v4l2_err(vd, "msm_alloc_one_entry failed");

			if (habmm_unexport(habmmhandle, export_id, 0))
				v4l2_err(vd, "failed to unexport id %d", export_id);

			export_id = 0;
		} else {
			v4l2_info(vd, "%s: alloc entry. buf type %d fd %d export_id %d sz %d",
				  __func__, buf_type, fd, export_id, algn_size);
		}
	}
#endif
exit:
	return export_id;
}

int msm_buf_put_export_id(struct virtio_video_stream* stream, uint32_t export_id,
			  enum virtio_video_event_type event_type, uint32_t flags)
{
	int ret = 0;
#ifdef NO_CACHE
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	uint32_t habmmhandle = vvd->commandq.vq->habmm_handle;
#else
	struct buf_export_entry* find_entry = NULL;
#endif

#ifndef NO_CACHE
	find_entry = get_entry_from_export_id(&stream->buf_cache.export_fifo, export_id);

	if (unlikely(!find_entry)) {
		pr_err("%s: failed to find an entry for export_id %d", __func__, export_id);
		ret = -EINVAL;
		goto exit;
	}

	if (VIRTIO_VIDEO_EVENT_FBD == event_type ||
	    VIRTIO_VIDEO_EVENT_EBD == event_type) {
		find_entry->is_export = false;
	}

exit:
#endif
	v4l2_info(&vvd->v4l2_dev, "%s: event_type=%#x, export_id=%#x, flags=%#x\n",
			__func__, event_type, export_id, flags);

	if (event_type == VIRTIO_VIDEO_EVENT_EBD ||
	    !(flags & VIRTIO_VIDEO_BUF_FLAG_READONLY)) {
		if (habmm_unexport(habmmhandle, export_id, 0))
			v4l2_err(&vvd->v4l2_dev, "%s: failed to unexport id %d",
				 __func__, export_id);
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
	cache->export_avail = MAX_NUM_EXPORT_CACHE_ENTRY;

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
	cache->export_avail = 0;
}
