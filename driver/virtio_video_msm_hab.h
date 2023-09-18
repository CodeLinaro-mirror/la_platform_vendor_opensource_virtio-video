/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _VIRTIO_VIDEO_MSM_HAB_H_
#define _VIRTIO_VIDEO_MSM_HAB_H_

#include "virtio_video.h"

int virtio_video_msm_queue_cmd_buffer(struct virtio_video_device* vvd,
	struct virtio_video_vbuffer* vbuf);

struct hab_vq_buffer {
	void* buf;
	struct list_head list;
};

enum hab_virtqueue_type {
	MSM_VIRTO_INV_TYPE,
	MSM_VIRTQ_CMD_TYPE,
	MSM_VIRTQ_EVT_TYPE,
};

struct hab_virtqueue {
	struct virtqueue vq;
	enum hab_virtqueue_type type;
	spinlock_t qlock;
	uint32_t habmm_handle;
	struct list_head vbuf_list;
	struct list_head resp_list;
	struct list_head unused_vq_buf_list;
	struct task_struct* resp_thread;
};

static inline struct hab_virtqueue *to_hab_vq(struct virtqueue *_vq)
{
	return container_of(_vq, struct hab_virtqueue, vq);
}

void msm_hab_sg_init_one(struct scatterlist *sg, const void *buf, unsigned int buflen);
int msm_hab_find_vqs(struct virtio_device *vdev, unsigned nvqs,
		     struct virtqueue *vqs[], vq_callback_t *callbacks[],
		     const char * const names[], const bool *ctx,
		     struct irq_affinity *desc);
void msm_hab_del_vqs(struct virtio_device *vdev);
void msm_hab_start(struct virtio_device *vdev);

int msm_hab_virtqueue_add_inbuf(struct virtqueue *vq,
				struct scatterlist sg[], unsigned int num,
				void *data, gfp_t gfp);
void* msm_hab_virtqueue_get_buf(struct virtqueue* vq, unsigned int* len);
void* msm_hab_virtqueue_detach_unused_buf(struct virtqueue* vq);
int msm_hab_virtqueue_add_sgs(struct virtqueue *vq,
			      struct scatterlist *sgs[], unsigned int out_sgs,
			      unsigned int in_sgs, void *data, gfp_t gfp);
#endif //_VIRTIO_VIDEO_MSM_HAB_H_
