/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef _MSM_VIDC_VB2_H_
#define _MSM_VIDC_VB2_H_

#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include "virtio_video.h"

struct vb2_queue *msm_vidc_get_vb2q(struct virtio_video_stream* stream,
				    u32 type, const char *func);

void *msm_vb2_attach_dmabuf(struct vb2_buffer *vb, struct device *dev,
			    struct dma_buf *dbuf, unsigned long size);
void msm_vb2_detach_dmabuf(void *buf_priv);
int msm_vb2_map_dmabuf(void *buf_priv);
void msm_vb2_unmap_dmabuf(void *buf_priv);

/* vb2_ops */
int msm_vidc_queue_setup(struct vb2_queue *queue,
			 unsigned int *num_buffers, unsigned int *num_planes,
			 unsigned int sizes[], struct device *alloc_devs[]);
int msm_vidc_start_streaming(struct vb2_queue *queue, unsigned int count);
void msm_vidc_stop_streaming(struct vb2_queue *queue);
void msm_vidc_buf_queue(struct vb2_buffer *vb2);
void msm_vidc_buf_cleanup(struct vb2_buffer *vb2);
int msm_vidc_buf_out_validate(struct vb2_buffer *vb2);
void msm_vidc_buf_request_complete(struct vb2_buffer *vb2);
int vb2q_init(struct virtio_video_stream* stream,
	      struct vb2_queue* queue, enum v4l2_buf_type type,
	      struct vb2_ops* ops, const struct vb2_mem_ops* mem_ops);
#endif // _MSM_VIDC_VB2_H_
