/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _VIRTIO_VIDEO_MSM_MEM_H_
#define _VIRTIO_VIDEO_MSM_MEM_H_

#include <linux/types.h>
#include "virtio_video.h"

int msm_buf_get_export_id(struct virtio_video_stream* stream,
                          uint64_t fd, uint32_t size,
                          enum virtio_video_queue_type queue_type);
int msm_buf_put_export_id(struct virtio_video_stream* stream, uint32_t export_id,
                          enum virtio_video_queue_type queue_type, bool cleanup);
int msm_buf_cleanup_buffers(struct virtio_video_stream* stream,
                            struct virtio_video_erased_buffers* buffers);
int msm_buf_put_export_queue_type(struct virtio_video_stream* stream,
                                  enum virtio_video_queue_type queue_type);
int msm_export_cache_init(struct virtio_video_stream* stream);
void msm_export_cache_destroy(struct virtio_video_stream* stream);
#endif //_VIRTIO_VIDEO_MSM_MEM_H_
