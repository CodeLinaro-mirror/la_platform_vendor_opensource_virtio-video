/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


static struct buf_export_entry*
get_entry_from_fd(struct list_head *fifo,
                  uint64_t fd, uint32_t size,
                  enum virtio_video_queue_type buf_type,
                  const char *tag)
{
	struct buf_export_entry *entry = NULL, *found = NULL;
	struct dma_buf *dmabuf = NULL;
	uint64_t inode = 0;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		vpr_e(tag, "%s: dma_buf_get error fd=0x%x", __func__, fd);
		goto exit;
	}

	inode = (uint64_t)dmabuf->file->f_inode;
	list_for_each_entry(entry, fifo, list) {
		if ((entry->inode == inode) &&
		    (entry->buf_type == buf_type) && (entry->size == size)) {
			found = entry;
			vpr_h(tag, "%s: fd %d, export_id=%d, inode %#x buffer_type %d, size %d",
			      __func__, fd, entry->export_id, inode,
			      buf_type, size);
			break;
		}
	}
	dma_buf_put(dmabuf);

exit:
	return found;
}

static struct buf_export_entry*
alloc_one_export_entry(struct buf_export_cache* cache,
                       uint64_t fd, uint32_t size,
                       enum virtio_video_queue_type buf_type,
                       uint32_t export_id,
                       const char *tag)
{
	struct buf_export_entry* entry = NULL;
	struct dma_buf *dmabuf = NULL;
	uint64_t inode = 0;

	if (cache->used_count >= MAX_NUM_EXPORT_CACHE_ENTRY) {
		vpr_e(tag, "%s: export cache is full, fd %d", __func__, fd);
		goto exit;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		vpr_e(tag, "%s: dma_buf_get error fd %d", __func__, fd);
		goto exit;
	}

	inode = (uint64_t)dmabuf->file->f_inode;

	entry = kmem_cache_alloc(cache->exports, GFP_KERNEL);
	if (entry) {
		memset(entry, 0, sizeof(*entry));

		entry->inode = inode;
		entry->export_id = export_id;
		entry->size = size;
		entry->buf_type = buf_type;

		list_add_tail(&entry->list, &cache->export_fifo);
		cache->used_count++;
		vpr_h(tag, "%s: fd %d export_id %d inode %#x dmabuf %#x size %d buf_type %d cache_size %d",
		      __func__, fd, export_id, inode, dmabuf,
		      size, buf_type, cache->used_count);
	}

	dma_buf_put(dmabuf);

exit:
	return entry;
}

uint32_t msm_buf_get_export_id(struct virtio_video_stream* stream,
			       uint64_t fd, uint32_t size,
			       enum virtio_video_queue_type buf_type)
{
	uint32_t habmmhandle = get_habmm_handle(stream);
	int ret = 0;
	uint32_t export_id = 0;
	const uint32_t export_flag = HABMM_EXPIMP_FLAGS_FD;
	int i = 0;
	struct buf_export_entry *entry = NULL;
	const char *tag = strm2tag(stream);

	if (!size) {
		vpr_e(tag, "%s: size 0. skip export", __func__);
		goto exit;
	}

	entry = get_entry_from_fd(&stream->buf_cache.export_fifo,
	                          fd, size, buf_type, tag);
	if (entry) {
		export_id = entry->export_id;
		goto exit;
	}

	//alloc an entry if no existing one
	for (i = 0; i < MAX_EXPORT_RETRY && ret != -ENOMEM; i++) {
		ret = habmm_export(habmmhandle, (void*)fd, size, &export_id, export_flag);
		if (ret) {
			vpr_e(tag, "%s: export failed. retry %d rc %d", __func__, i, ret);
		} else {
			vpr_h(tag, "%s: export ok, type %d fd %d export_id %d sz %d retry %d",
			      __func__, buf_type, fd, export_id, size, i);
			break;
		}
	}

	if (unlikely(ret) || unlikely(!export_id)) {
		vpr_e(tag, "%s: export failed. buf type %d fd %d sz %d", __func__,
		      buf_type, fd, size);
		export_id = 0;
		goto exit;
	}

	entry = alloc_one_export_entry(&stream->buf_cache, fd, size,
				       buf_type, export_id, tag);
	if (!entry) {
		vpr_e(tag, "alloc entry failed");

		if (habmm_unexport(habmmhandle, export_id, 0))
			vpr_e(tag, "failed to unexport id %d", export_id);

		export_id = 0;
	} else {
		vpr_h(tag, "%s: alloc entry. buf type %d fd %d export_id %d sz %d",
		      __func__, buf_type, fd, export_id, size);
	}

exit:
	return export_id;
}

int msm_buf_put_export_id(struct virtio_video_stream* stream, uint32_t export_id)
{
	int ret = 0;
	uint32_t habmmhandle = get_habmm_handle(stream);

	ret = habmm_unexport(habmmhandle, export_id, 0);
	if (ret)
		vpr_e(strm2tag(stream), "%s: failed to unexport id %d, ret=%d",
		      __func__, export_id, ret);
	else
		vpr_h(strm2tag(stream), "%s: unexport ok, export_id=%d",
		      __func__, export_id);

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
	cache->used_count = 0;

exit:
	return ret;
}

void msm_export_cache_destroy(struct virtio_video_stream* stream)
{
	struct buf_export_entry* entry = NULL, *temp = NULL;
	struct buf_export_cache* cache = &stream->buf_cache;

	list_for_each_entry_safe(entry, temp, &cache->export_fifo, list) {
		list_del(&entry->list);
		msm_buf_put_export_id(stream, entry->export_id);
		kmem_cache_free(cache->exports, entry);
	}
	kmem_cache_destroy(cache->exports);
	cache->used_count = 0;
}
