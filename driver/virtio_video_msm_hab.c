/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/kthread.h>
#include <linux/kernel.h>
#include "virtio_video_msm_hab.h"
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include "../../../drivers/soc/qcom/hab/hab_virtio.h"
#include <media/v4l2-common.h>
#include "virtio_video.h"

#pragma GCC diagnostic ignored  "-Wunused-function"

int virtio_video_msm_hab_open(struct virtio_video_device* vvd)
{
	int ret = 0;
	uint32_t* ph = NULL;

	ph = &vvd->commandq.vq->habmm_handle;
	ret = habmm_socket_open(ph, MM_VID, 0, 0);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev,"habmm command socket open failed %d", ret);
		return ret;
	}
	v4l2_info(&vvd->v4l2_dev, "commandq hab open done, handle %x", *ph);

	ph = &vvd->eventq.vq->habmm_handle;
	ret = habmm_socket_open(ph, MM_VID, 0, 0);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev,"habmm event socket open failed %d", ret);
		return ret;
	}
	v4l2_info(&vvd->v4l2_dev, "eventq hab open done, handle %x", *ph);

	return ret;
}

void virtio_video_msm_hab_close(struct virtio_video_device* vvd)
{
	int ret = 0;

	if (vvd->commandq.vq->habmm_handle) {
		ret = habmm_socket_close(vvd->commandq.vq->habmm_handle);
		if (ret) {
			v4l2_err(&vvd->v4l2_dev, "habmm command socket close failed %d", ret);
		}
	}

	if (vvd->eventq.vq->habmm_handle) {
		ret = habmm_socket_close(vvd->eventq.vq->habmm_handle);
		if (ret) {
			v4l2_err(&vvd->v4l2_dev, "habmm event socket close failed %d", ret);
		}
	}
}

void* msm_hab_virtqueue_get_buf(struct msm_hab_virtqueue* vq, unsigned int* len)
{
	struct hab_vq_buffer* entry = NULL;
	void* buf = NULL;

	entry = list_first_entry_or_null(&vq->resp_list,
		struct hab_vq_buffer, list);

	if (entry) {
		list_del(&entry->list);
		buf = entry->buf;
		kfree(entry);
	}

	return buf;
}

void* msm_hab_virtqueue_detach_unused_buf(struct msm_hab_virtqueue* vq)
{
	return NULL;
}

int msm_hab_virtqueue_add_sgs(struct msm_hab_virtqueue *vq,
					struct scatterlist *sgs[],
					unsigned int out_sgs,
					unsigned int in_sgs,
					void *data,
					gfp_t gfp)
{
	return 0;
}

int msm_hab_virtqueue_add_inbuf(struct msm_hab_virtqueue *vq,
					struct scatterlist sg[],
					unsigned int num,
					void *data,
					gfp_t gfp)
{
	return 0;
}
