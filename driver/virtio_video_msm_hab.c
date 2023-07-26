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

#define SESSION_ERROR -1
#define DEFAULT_VQ_NUM 128

static int start_resp_handler(struct hab_virtqueue *hvq);
static void stop_cmd_resp_handler(struct hab_virtqueue* hvq);
static void stop_event_handler(struct hab_virtqueue* hvq);

static int attach_buf_to_vq_buf(struct hab_virtqueue *hvq,
                                 struct list_head *ls, void *data)
{
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct v4l2_device *v4l2_dev = &vvd->v4l2_dev;
	struct hab_vq_buffer *vq_buf = NULL;
	int ret = 0;

	spin_lock(&hvq->qlock);
	vq_buf = list_first_entry_or_null(&hvq->unused_vq_buf_list,
	                                  struct hab_vq_buffer, list);
	if (vq_buf) {
		list_del(&vq_buf->list);
		vq_buf->buf = data;
		list_add_tail(&vq_buf->list, ls);
	} else {
		v4l2_err(v4l2_dev, "%s: no available vq_buf\n", __func__);
		ret = -ENOENT;
	}

	spin_unlock(&hvq->qlock);

	return ret;
}

static void* unattach_buf_from_vq_buf(struct hab_virtqueue *hvq,
                                      struct list_head *ls)
{
	struct hab_vq_buffer *vq_buf = NULL;
	void *data = NULL;

	spin_lock(&hvq->qlock);

	vq_buf = list_first_entry_or_null(ls, struct hab_vq_buffer, list);

	if (vq_buf) {
		list_del(&vq_buf->list);
		list_add_tail(&vq_buf->list, &hvq->unused_vq_buf_list);
		data = vq_buf->buf;
		vq_buf->buf = NULL;
	}

	spin_unlock(&hvq->qlock);

	return data;
}

static int virtio_video_msm_hab_open(struct hab_virtqueue *hvq)
{
	int ret = 0;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct v4l2_device *v4l2_dev = &vvd->v4l2_dev;
	int mmid = 0;

	mmid = (vvd->type == VIRTIO_VIDEO_DEVICE_DECODER) ? MM_VID : MM_VID_2;

	v4l2_info(v4l2_dev, "%s: %s mmid=%d\n", __func__, hvq->vq.name, mmid);

	ret = habmm_socket_open(&hvq->habmm_handle, mmid, 0, 0);
	if (ret) {
		v4l2_err(v4l2_dev, "%s: %s failed %d\n",
		         __func__, hvq->vq.name, ret);
		goto err;
	}

	ret = start_resp_handler(hvq);
	if (ret) {
		goto err_start_handler;
	}

	v4l2_info(v4l2_dev, "%s: %s mmid=%d done, socket 0x%x\n",
	          __func__, hvq->vq.name, mmid, hvq->habmm_handle);

	return 0;

err_start_handler:
	habmm_socket_close(hvq->habmm_handle);
	hvq->habmm_handle = 0;
err:
	return ret;
}

static void virtio_video_msm_hab_close(struct hab_virtqueue* hvq)
{
	int ret = 0;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct v4l2_device *v4l2_dev = &vvd->v4l2_dev;

	if (hvq->type == MSM_VIRTQ_CMD_TYPE && hvq->habmm_handle) {
		ret = habmm_socket_close(hvq->habmm_handle);
		if (ret)
			v4l2_err(v4l2_dev, "%s, habmm cmd socket close failed %d\n",
			         __func__, ret);
		stop_cmd_resp_handler(hvq);
	}

	if (hvq->type == MSM_VIRTQ_EVT_TYPE && hvq->habmm_handle) {
		ret = habmm_socket_close(hvq->habmm_handle);
		if (ret)
			v4l2_err(v4l2_dev, "%s: habmm event socket close failed %d\n",
			         __func__, ret);
		stop_event_handler(hvq);
	}

	v4l2_info(v4l2_dev, "%s: %s socket=0x%x done\n", __func__,
	          hvq->vq.name, hvq->habmm_handle);

	hvq->habmm_handle = 0;
}

static void stop_cmd_resp_handler(struct hab_virtqueue* hvq)
{
	struct hab_vq_buffer *entry = NULL, *tmp = NULL;
	/** shut down the callback loop and free memory **/
	if (hvq->resp_thread) {
		kthread_stop(hvq->resp_thread);
		hvq->resp_thread = NULL;
	}

	list_for_each_entry_safe(entry, tmp, &hvq->resp_list, list) {
		list_del(&entry->list);
		kfree(entry);
	}
}

static void stop_event_handler(struct hab_virtqueue *hvq)
{
	if (hvq->resp_thread) {
		kthread_stop(hvq->resp_thread);
		hvq->resp_thread = NULL;
	}
}

static int process_msm_hab_cmd_resp(struct hab_virtqueue* hvq, void* data)
{
	int found = 0;
	struct virtio_video_vbuffer *vbuf = NULL, *vbuf_tmp = NULL;
	struct virtqueue *vq = &hvq->vq;
	struct virtio_video_device* vvd = hvq->vq.vdev->priv;
	struct v4l2_device *v4l2_dev = &vvd->v4l2_dev;
	struct virtio_video_cmd_hdr *hdr = (struct virtio_video_cmd_hdr *)data;
	void *resp = NULL;
	int ret = 0;
	int resp_rc = 0;

	if (hdr->type == SESSION_ERROR) {
		v4l2_err(v4l2_dev, "%s: session error\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	spin_lock(&vvd->commandq.qlock);

	list_for_each_entry_safe(vbuf, vbuf_tmp,
				&vvd->pending_vbuf_list, pending_list_entry) {

		struct virtio_video_cmd_hdr* vbuf_hdr
			= (struct virtio_video_cmd_hdr*)vbuf->buf;

		if (vbuf_hdr->stream_id == hdr->stream_id  &&
		    vbuf_hdr->type == hdr->type) {
			found = 1;
			break;
		}
	}

	if (found) {
		resp = (uint8_t*)data + vbuf->size + vbuf->data_size;
		resp_rc = ((struct virtio_video_resp *)resp)->result;

		if (vbuf->resp_buf && vbuf->resp_size) {
			resp += sizeof(struct virtio_video_resp);
			memcpy(vbuf->resp_buf, resp, vbuf->resp_size);
		}

		v4l2_info(v4l2_dev, "%s: recv: cmd_type=%s. resp=%s\n",
		          __func__,
		          cmd_to_string(hdr->type),
		          cmd_to_string(resp_rc));

		attach_buf_to_vq_buf(hvq, &hvq->resp_list, vbuf);
	}
	else {
		v4l2_err(v4l2_dev, "%s: unable find pending vbuf stream_id=%d\n",
		         __func__, hdr->stream_id);
		ret = -EINVAL;
	}

	spin_unlock(&vvd->commandq.qlock);

	if (found)
		virtio_video_cmd_cb(vq);

err:
	return ret;
}

static int process_msm_hab_evt_resp(struct hab_virtqueue* hvq, void* data)
{
	struct virtio_video_event *evt = (struct virtio_video_event *)data;
	struct virtqueue *vq = &hvq->vq;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct v4l2_device *v4l2_dev = &vvd->v4l2_dev;
	int ret = 0;

	v4l2_info(v4l2_dev, "%s: recv: event type %#x, stream id %d\n",
	          __func__, evt->event_type, evt->stream_id);

	ret = attach_buf_to_vq_buf(hvq, &hvq->resp_list, evt);

	if (!ret)
		virtio_video_event_cb(vq);

	return ret;
}

static int virtio_video_hab_resp_handler(void* p)
{
	struct hab_virtqueue* hvq = p;
	struct virtio_video_device* vvd = hvq->vq.vdev->priv;
	struct v4l2_device *v4l2_dev = &vvd->v4l2_dev;
	const char *vq_name = hvq->vq.name;
	uint8_t buf[MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE] = {0};
	int size_bytes = 0;
	void *msg = NULL;
	int ret = 0;

	v4l2_info(v4l2_dev, "%s %s: start\n", vq_name, __func__);

	while (!kthread_should_stop()) {

		if (hvq->type == MSM_VIRTQ_EVT_TYPE) {
			msg = unattach_buf_from_vq_buf(hvq, &hvq->vbuf_list);
			if (unlikely(!msg)) {
				v4l2_err(v4l2_dev, "%s %s: unable get event buffer\n",
				         vq_name, __func__);
				goto err;
			}
			size_bytes = sizeof(struct virtio_video_event);
		} else {
			msg = buf;
			size_bytes = sizeof(buf);
		}

		memset(msg, 0, size_bytes);
		ret = habmm_socket_recv(hvq->habmm_handle,
		                        msg, &size_bytes, 0, 0);

		if (unlikely(ret)) {
			if (-EINTR == ret) {
				continue;
			}
			else if (-ENODEV == ret) {
				v4l2_info(v4l2_dev, "%s %s: socket 0x%x closed\n",
				          vq_name, __func__, hvq->habmm_handle);
				goto exit;
			}
			else {
				v4l2_err(v4l2_dev, "%s %s: socket recv failed: rc=%d\n",
				         vq_name, __func__, ret);
				goto err;
			}
		}

		if (hvq->type == MSM_VIRTQ_CMD_TYPE)
			ret = process_msm_hab_cmd_resp(hvq, msg);
		else
			ret = process_msm_hab_evt_resp(hvq, msg);

		if (ret)
			goto err;
	}

exit:
	return 0;
err:
	v4l2_err(v4l2_dev, "%s %s: exited. error %d\n", vq_name, __func__, ret);
	return ret;
}

static int start_resp_handler(struct hab_virtqueue *hvq)
{
	int ret = 0;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct v4l2_device *v4l2_dev = &vvd->v4l2_dev;

	hvq->resp_thread = kthread_create(virtio_video_hab_resp_handler,
	                                  hvq, "vvid_rsp_%s", hvq->vq.name);

	if (IS_ERR(hvq->resp_thread)) {
		v4l2_err(v4l2_dev, "failed to create %s handler thread\n",
			 hvq->vq.name);
		ret = PTR_ERR(hvq->resp_thread);
	}

	return ret;
}

bool msm_hab_virtqueue_kick(struct virtqueue *vq)
{
	struct hab_virtqueue * hvq = to_hab_vq(vq);
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	uint32_t habmm_handle = hvq->habmm_handle;
	struct virtio_video_msg *msg = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	unsigned long flags = 0L;
	int ret = 0;

	if (hvq->type == MSM_VIRTQ_CMD_TYPE) {
		vbuf = unattach_buf_from_vq_buf(hvq, &hvq->vbuf_list);
		msg = (struct virtio_video_msg *)vbuf->buf;

		v4l2_info(&vvd->v4l2_dev, "%s: socket 0x%x: type %s, stream_id 0x%x, %d %d %d\n",
		          __func__, habmm_handle, cmd_to_string(msg->hdr.type),
		          msg->hdr.stream_id, vbuf->size, vbuf->data_size, vbuf->resp_size);

		spin_unlock_irqrestore(&vvd->commandq.qlock, flags);

		ret = habmm_socket_send(habmm_handle, msg, sizeof(*msg), 0);

		spin_lock_irqsave(&vvd->commandq.qlock, flags);

		if (ret)
			v4l2_err(&vvd->v4l2_dev, "%s: habmm_socket_send failed %d\n",
			         __func__, ret);
	}

	return true;
}

void msm_hab_sg_init_one(struct scatterlist *sg, const void *buf,
                         unsigned int buflen)
{
}

void* msm_hab_virtqueue_get_buf(struct virtqueue* vq, unsigned int* len)
{
	struct hab_virtqueue * hvq = to_hab_vq(vq);
	void *buf = NULL;

	buf = unattach_buf_from_vq_buf(hvq, &hvq->resp_list);

	return buf;
}

void* msm_hab_virtqueue_detach_unused_buf(struct virtqueue* vq)
{
	struct hab_vq_buffer *vq_buf = NULL, *tmp = NULL;
	struct hab_virtqueue *hvq = to_hab_vq(vq);
	void *buf = NULL;

	buf = unattach_buf_from_vq_buf(hvq, &hvq->vbuf_list);

	if (!buf)
		buf = unattach_buf_from_vq_buf(hvq, &hvq->resp_list);

	if (!buf) {
		spin_lock(&hvq->qlock);
		list_for_each_entry_safe(vq_buf, tmp,
		                         &hvq->unused_vq_buf_list, list) {
			list_del(&vq_buf->list);
			kfree(vq_buf);
		}
		spin_unlock(&hvq->qlock);
	}

	return buf;
}

int msm_hab_virtqueue_add_sgs(struct virtqueue *vq, struct scatterlist *sgs[],
                              unsigned int out_sgs, unsigned int in_sgs,
                              void *data, gfp_t gfp)
{
	struct hab_virtqueue *hvq = to_hab_vq(vq);
	const char *name = dev_name(&vq->vdev->dev);
	struct hab_vq_buffer *vq_buf = NULL;

	spin_lock(&hvq->qlock);
	vq_buf = list_first_entry_or_null(&hvq->unused_vq_buf_list,
	                                  struct hab_vq_buffer, list);

	if (vq_buf)
		list_del(&vq_buf->list);

	spin_unlock(&hvq->qlock);

	if (!vq_buf) {
		if (!(vq_buf = kmalloc(sizeof(*vq_buf), GFP_KERNEL))) {
			pr_err("%s: %s out of memory\n", name, __func__);
			goto err;
		}
	}

	vq_buf->buf = data;

	spin_lock(&hvq->qlock);
	list_add_tail(&vq_buf->list, &hvq->vbuf_list);
	spin_unlock(&hvq->qlock);

	return 0;
err:
	return -ENOMEM;
}

int msm_hab_virtqueue_add_inbuf(struct virtqueue *vq, struct scatterlist sg[],
                                unsigned int num, void *data, gfp_t gfp)
{
	(void)sg;
	return msm_hab_virtqueue_add_sgs(vq, NULL, 0, num, data, gfp);
}

int msm_hab_find_vqs(struct virtio_device *vdev, unsigned nvqs,
                     struct virtqueue *vqs[], vq_callback_t *callbacks[],
                     const char * const names[], const bool *ctx,
                     struct irq_affinity *desc)
{
	int ret = 0;
	struct hab_virtqueue *hvq = NULL;
	int i = 0;

	pr_info("%s: %s\n", dev_name(&vdev->dev), __func__);

	for (i = 0; i < nvqs; i++) {
		if (!(hvq = devm_kzalloc(&vdev->dev, sizeof(*hvq), GFP_KERNEL))) {
			ret = -ENOMEM;
			goto err;
		}

		hvq->vq.callback = callbacks[i];
		hvq->vq.name = names[i];
		hvq->vq.vdev = vdev;
		hvq->vq.num_free = DEFAULT_VQ_NUM;
		hvq->habmm_handle = 0;
		spin_lock_init(&hvq->qlock);
		INIT_LIST_HEAD(&hvq->vbuf_list);
		INIT_LIST_HEAD(&hvq->resp_list);
		INIT_LIST_HEAD(&hvq->unused_vq_buf_list);

		if (!strcmp(names[i], "commandq"))
			hvq->type = MSM_VIRTQ_CMD_TYPE;
		else
			hvq->type = MSM_VIRTQ_EVT_TYPE;

		ret = virtio_video_msm_hab_open(hvq);
		if (ret)
			goto err;

		vqs[i] = &hvq->vq;

		spin_lock(&vdev->vqs_list_lock);
		list_add_tail(&hvq->vq.list, &vdev->vqs);
		spin_unlock(&vdev->vqs_list_lock);
	}

	return 0;

err:
	msm_hab_del_vqs(vdev);
	return ret;
}

void msm_hab_del_vqs(struct virtio_device *vdev)
{
	struct virtqueue *vq = NULL, *n = NULL;
	struct hab_virtqueue *hvq = NULL;

	pr_info("%s: %s\n", dev_name(&vdev->dev), __func__);

	list_for_each_entry_safe(vq, n, &vdev->vqs, list) {
		hvq = to_hab_vq(vq);
		virtio_video_msm_hab_close(hvq);
	}
}


void msm_hab_start(struct virtio_device *vdev)
{
	struct virtqueue *entry = NULL, *tmp = NULL;
	struct hab_virtqueue *hvq = NULL;

	spin_lock(&vdev->vqs_list_lock);
	list_for_each_entry_safe(entry, tmp, &vdev->vqs, list) {
		hvq = to_hab_vq(entry);
		wake_up_process(hvq->resp_thread);
	}
	spin_unlock(&vdev->vqs_list_lock);
}
