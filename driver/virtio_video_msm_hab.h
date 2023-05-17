/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _VIRTIO_VIDEO_MSM_HAB_H_
#define _VIRTIO_VIDEO_MSM_HAB_H_

#include "virtio_video.h"

int virtio_video_msm_queue_cmd_buffer(struct virtio_video_device* vvd,
	struct virtio_video_vbuffer* vbuf);

int virtio_video_msm_hab_open(struct virtio_video_device* vvd);
void virtio_video_msm_hab_close(struct virtio_video_device* vvd);

struct hab_vq_buffer {
	void* buf;
	struct list_head list;
};

int msm_hab_virtqueue_add_inbuf(struct msm_hab_virtqueue *vq,
				struct scatterlist sg[], unsigned int num,
				void *data, gfp_t gfp);
void* msm_hab_virtqueue_get_buf(struct msm_hab_virtqueue* vq, unsigned int* len);
void* msm_hab_virtqueue_detach_unused_buf(struct msm_hab_virtqueue* vq);
int msm_hab_virtqueue_add_sgs(struct msm_hab_virtqueue *vq,
			      struct scatterlist *sgs[], unsigned int out_sgs,
			      unsigned int in_sgs, void *data, gfp_t gfp);
#endif //_VIRTIO_VIDEO_MSM_HAB_H_
