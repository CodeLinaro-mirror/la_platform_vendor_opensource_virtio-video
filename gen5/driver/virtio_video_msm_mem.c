/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
#define MAX_NUM_EXPORT_CACHE_ENTRY 256


struct buf_export_entry {
	struct list_head list;
	uint64_t inode;
	uint32_t size;
	uint32_t export_id;
	bool in_use;
	enum virtio_video_queue_type queue_type;
};

static struct buf_export_entry*
get_entry_from_fd(struct buf_export_cache* cache,
                  uint64_t fd, uint32_t size,
                  enum virtio_video_queue_type queue_type,
                  const char *tag)
{
	struct buf_export_entry *entry = NULL, *found = NULL;
	struct dma_buf *dmabuf = NULL;
	uint64_t inode = 0;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		vpr_e(tag, "%s: dma_buf_get error fd=%llu", __func__, fd);
		goto exit;
	}

	inode = (uint64_t)dmabuf->file->f_inode;

	mutex_lock(&cache->lock);
	list_for_each_entry(entry, &cache->export_fifo, list) {
		if ((entry->inode == inode) &&
		    (entry->queue_type == queue_type) && (entry->size == size)) {
			found = entry;
			entry->in_use = true;
			vpr_h(tag, "%s: fd %llu export_id=%d inode %#llx queue_type %2d, sz %d",
			      __func__, fd, entry->export_id, inode,
			      queue_type, size);
			break;
		}
	}
	mutex_unlock(&cache->lock);

	dma_buf_put(dmabuf);

exit:
	return found;
}

static struct buf_export_entry*
alloc_one_export_entry(struct buf_export_cache* cache,
                       uint64_t fd, uint32_t size,
                       enum virtio_video_queue_type queue_type,
                       uint32_t export_id,
                       const char *tag)
{
	struct buf_export_entry* entry = NULL;
	struct dma_buf *dmabuf = NULL;
	uint64_t inode = 0;

	if (cache->used_count >= MAX_NUM_EXPORT_CACHE_ENTRY) {
		vpr_e(tag, "%s: export cache full(%d>=%d), fd %llu", __func__,
		      cache->used_count, MAX_NUM_EXPORT_CACHE_ENTRY, fd);
		goto exit;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		vpr_e(tag, "%s: dma_buf_get error fd %llu", __func__, fd);
		goto exit;
	}

	inode = (uint64_t)dmabuf->file->f_inode;

	entry = kmem_cache_alloc(cache->exports, GFP_KERNEL);
	if (entry) {
		memset(entry, 0, sizeof(*entry));

		entry->inode = inode;
		entry->export_id = export_id;
		entry->size = size;
		entry->queue_type = queue_type;
		entry->in_use = true;

		mutex_lock(&cache->lock);
		list_add_tail(&entry->list, &cache->export_fifo);
		cache->used_count++;
		mutex_unlock(&cache->lock);

		vpr_h(tag, "%s: fd %3lld export_id %3d inode %#llx dmabuf %pK queue_type %2d cache count %2d sz %d",
		      __func__, fd, export_id, inode, dmabuf,
		      queue_type, cache->used_count, size);
	}

	dma_buf_put(dmabuf);

exit:
	return entry;
}

int msm_buf_get_export_id(struct virtio_video_stream* stream,
                          uint64_t fd, uint32_t size,
                          enum virtio_video_queue_type queue_type)
{
	uint32_t habmmhandle = get_habmm_handle(stream);
	int ret = 0;
	uint32_t export_id = 0;
	const uint32_t exp_flag = HABMM_EXPIMP_FLAGS_FD;
	int i = 0;
	struct buf_export_entry *entry = NULL;
	const char *tag = strm2tag(stream);

	if (!size) {
		vpr_e(tag, "%s: size 0. skip export", __func__);
		goto exit;
	}

	entry = get_entry_from_fd(&stream->buf_cache, fd, size, queue_type, tag);
	if (entry) {
		export_id = entry->export_id;
		goto exit;
	}

	//alloc an entry if no existing one
	for (i = 0; i < MAX_EXPORT_RETRY && ret != -ENOMEM; i++) {
		ret = habmm_export(habmmhandle, (void*)fd, size, &export_id, exp_flag);
		if (!ret) {
			vpr_h(tag, "%s: export ok, queue_type %2d fd %3llu export_id %3d sz %d",
			      __func__, queue_type, fd, export_id, size);
			break;
		}
	}

	if (unlikely(ret) || unlikely(!export_id)) {
		vpr_e(tag, "%s: export failed. queue_type %3d fd %3llu sz %d", __func__,
		      queue_type, fd, size);
		export_id = 0;
		goto exit;
	}

	entry = alloc_one_export_entry(&stream->buf_cache, fd, size,
	                               queue_type, export_id, tag);
	if (!entry) {
		vpr_e(tag, "alloc entry failed");

		if (habmm_unexport(habmmhandle, export_id, 0))
			vpr_e(tag, "failed to unexport id %d", export_id);

		export_id = 0;
	} else {
		vpr_h(tag, "%s: alloc entry. queue_type %2d fd %3llu export_id %d sz %d",
		      __func__, queue_type, fd, export_id, size);
	}

exit:
	return export_id;
}

static inline int msm_buf_unexport(struct virtio_video_stream* stream,
                                   uint32_t export_id)
{
	int ret = 0;
	uint32_t habmmhandle = get_habmm_handle(stream);

	ret = habmm_unexport(habmmhandle, export_id, 0);
	if (ret)
		vpr_e(strm2tag(stream), "%s: failed to unexport id %d, ret=%d",
		      __func__, export_id, ret);
	else
		vpr_h(strm2tag(stream), "%s: unexport ok, export_id %d", __func__,
		      export_id);

	return ret;
}

static inline void msm_buf_free_cache_entry(struct virtio_video_stream* stream,
                                            struct buf_export_entry *entry)
{
	struct buf_export_cache* cache = &stream->buf_cache;

	vpr_l(strm2tag(stream), "%s: export_id %d, cache released, cache count %d",
	      __func__, entry->export_id, cache->used_count);

	list_del(&entry->list);
	kmem_cache_free(cache->exports, entry);
	cache->used_count--;

	return;
}

int msm_buf_put_export_id(struct virtio_video_stream* stream,
                          uint32_t export_id,
                          enum virtio_video_queue_type queue_type,
                          bool cleanup)
{
	int ret = 0;
	struct buf_export_entry* entry = NULL, *temp = NULL;
	struct buf_export_cache* cache = &stream->buf_cache;

	if (cleanup) {
		ret = msm_buf_unexport(stream, export_id);
		if (ret) {
			vpr_e(strm2tag(stream), "%s: failed, export_id %d", __func__, export_id);
			goto exit;
		}
	}

	mutex_lock(&cache->lock);
	list_for_each_entry_safe(entry, temp, &cache->export_fifo, list) {
		if (entry->export_id == export_id) {
			vpr_l(strm2tag(stream), "%s: cleanup %d queue_type %2d export_id %d inode %#llx",
			      __func__, cleanup, queue_type, export_id, entry->inode);
			if (cleanup)
				msm_buf_free_cache_entry(stream, entry);
			else
				entry->in_use = false;
			break;
		}
	}
	mutex_unlock(&cache->lock);

exit:
	return ret;
}

int msm_buf_cleanup_buffers(struct virtio_video_stream* stream,
                            struct virtio_video_erased_buffers* buffers)
{
	int ret = 0;
	uint32_t idx = 0;
	uint32_t count = buffers->count;
	uint32_t export_id = 0;
	struct buf_export_entry* entry = NULL, *temp = NULL;
	struct buf_export_cache* cache = &stream->buf_cache;

	for (idx = 0; idx < count; idx++)
	{
		export_id = buffers->export_ids[idx];
		if (!export_id)
		{
			vpr_h(strm2tag(stream), "%s: invalid export_id %d",
			      __func__, export_id);
			continue;
		}

		mutex_lock(&cache->lock);
		list_for_each_entry_safe(entry, temp, &cache->export_fifo, list) {
			if (entry->export_id == export_id && !entry->in_use) {
				ret = msm_buf_unexport(stream, entry->export_id);
				if (ret) {
					vpr_e(strm2tag(stream), "%s: failed to unexport export_id %d",
					      __func__, export_id);
					break;
				}
				vpr_l(strm2tag(stream), "%s: cleanup buffer, queue_type %2d, export_id %d",
				      __func__, entry->queue_type, export_id);
				msm_buf_free_cache_entry(stream, entry);

			}
		}
		mutex_unlock(&cache->lock);
	}

	return ret;
}

int msm_buf_put_export_queue_type(struct virtio_video_stream* stream,
                                  enum virtio_video_queue_type queue_type)
{
	int ret = 0;
	struct buf_export_entry* entry = NULL, *temp = NULL;
	struct buf_export_cache* cache = &stream->buf_cache;
	vpr_l(strm2tag(stream), "%s: queue_type %2d", __func__, queue_type);

	mutex_lock(&cache->lock);
	list_for_each_entry_safe(entry, temp, &cache->export_fifo, list) {
		if (entry->queue_type == queue_type) {
			ret = msm_buf_unexport(stream, entry->export_id);
			msm_buf_free_cache_entry(stream, entry);
		}
	}
	mutex_unlock(&cache->lock);

	return ret;
}

int msm_export_cache_init(struct virtio_video_stream* stream)
{
	struct buf_export_cache* cache = &stream->buf_cache;
	int ret = 0;

	cache->exports = kmem_cache_create("virtio-video-exp-cache",
	                              sizeof(struct buf_export_entry), 0, 0, NULL);
	if (!cache->exports) {
		ret = -ENOMEM;
		goto exit;
	}

	INIT_LIST_HEAD(&cache->export_fifo);
	mutex_init(&cache->lock);
	cache->used_count = 0;

exit:
	return ret;
}

void msm_export_cache_destroy(struct virtio_video_stream* stream)
{
	struct buf_export_entry* entry = NULL, *temp = NULL;
	struct buf_export_cache* cache = &stream->buf_cache;

	mutex_lock(&cache->lock);
	list_for_each_entry_safe(entry, temp, &cache->export_fifo, list) {
		msm_buf_unexport(stream, entry->export_id);
		msm_buf_free_cache_entry(stream, entry);
	}
	mutex_unlock(&cache->lock);

	mutex_destroy(&cache->lock);
	kmem_cache_destroy(cache->exports);
	cache->used_count = 0;
}
