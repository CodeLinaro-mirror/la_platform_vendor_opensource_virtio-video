/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _VIRTIO_VIDEO_MSM_MEM_H_
#define _VIRTIO_VIDEO_MSM_MEM_H_

#include <linux/types.h>
#include "virtio_video.h"

uint32_t msm_buf_get_export_id(struct virtio_video_stream* stream,
			       uint32_t habmmhandle, uint64_t fd, uint32_t size,
			       enum virtio_video_queue_type buf_type, bool export_as_fd);
int msm_buf_put_export_id(struct virtio_video_stream* stream, uint32_t export_id,
			  uint32_t frame_flag, enum virtio_video_event_type event_type);
int msm_export_cache_init(struct buf_export_cache* cache);
void msm_export_cache_destroy(struct buf_export_cache* cache);
#endif //_VIRTIO_VIDEO_MSM_MEM_H_
