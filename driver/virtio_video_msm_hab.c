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

static int start_resp_handler(struct hab_virtqueue *hvq);
static void stop_cmd_resp_handler(struct hab_virtqueue* hvq);
static void stop_event_handler(struct hab_virtqueue* hvq);

int virtio_video_msm_queue_cmd_buffer(struct virtio_video_device* vvd,
	struct virtio_video_vbuffer* vbuf)
{
	int ret = 0;
	struct hab_virtqueue *hvq = to_hab_vq(vvd->commandq.vq);
	uint32_t habmm_handle = hvq->habmm_handle;
	struct virtio_video_msg msg = {0};
	uint8_t* pmsg = (uint8_t *)&msg;

	spin_lock(&vvd->commandq.qlock);

	vbuf->id = vvd->vbufs_sent++;
	list_add_tail(&vbuf->pending_list_entry, &vvd->pending_vbuf_list);

	memcpy(pmsg, vbuf->buf, vbuf->size);
	pmsg += vbuf->size;

	if (vbuf->data_size) {
		memcpy(pmsg, vbuf->data_buf, vbuf->data_size);
		pmsg += vbuf->data_size;
	}

	if (vbuf->resp_size) {
		memcpy(pmsg, vbuf->resp_buf, vbuf->resp_size);
		pmsg += vbuf->data_size;
	}

	v4l2_info(&vvd->v4l2_dev, "%s: socket 0x%x: type %s, stream_id 0x%x\n",
	          __func__, habmm_handle, cmd_to_string(msg.hdr.type),
	          msg.hdr.stream_id);

	ret = habmm_socket_send(habmm_handle, &msg, sizeof(msg), 0);
	if (ret)
		v4l2_err(&vvd->v4l2_dev, "%s: habmm_socket_send failed %d\n",
		         __func__, ret);

	spin_unlock(&vvd->commandq.qlock);

	return ret;
}

static int virtio_video_msm_hab_open(struct hab_virtqueue *hvq)
{
	int ret = 0;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	int mmid = 0;

	mmid = (vvd->type == VIRTIO_VIDEO_DEVICE_DECODER) ? MM_VID : MM_VID_2;

	v4l2_info(&vvd->v4l2_dev, "%s %s mmid=%d\n", __func__, hvq->vq.name, mmid);

	ret = habmm_socket_open(&hvq->habmm_handle, mmid, 0, 0);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s %s failed %d\n",
			 __func__, hvq->vq.name, ret);
		goto err;
	}

	ret = start_resp_handler(hvq);
	if (ret) {
		goto err_start_handler;
	}

	v4l2_info(&vvd->v4l2_dev, "%s %s mmid=%d done, socket 0x%x\n",
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

	if (hvq->type == MSM_VIRTQ_CMD_TYPE && hvq->habmm_handle) {
		ret = habmm_socket_close(hvq->habmm_handle);
		if (ret)
			pr_err("habmm command socket close failed %d\n", ret);
		stop_cmd_resp_handler(hvq);
	}

	if (hvq->type == MSM_VIRTQ_EVT_TYPE && hvq->habmm_handle) {
		ret = habmm_socket_close(hvq->habmm_handle);
		if (ret)
			pr_err("habmm event socket close failed %d\n", ret);
		stop_event_handler(hvq);
	}

	v4l2_info(&vvd->v4l2_dev, "%s %s socket=0x%x done\n", __func__,
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

static int process_msm_hab_cmd_resp(struct hab_virtqueue* hvq, uint8_t* data)
{
	int found = 0;
	struct virtio_video_vbuffer *vbuf = NULL, *vbuf_tmp = NULL;
	struct virtio_video_resp *resp = NULL;
	struct hab_vq_buffer *vq_buf = NULL;
	struct virtqueue *vq = &hvq->vq;
	struct virtio_video_device* vvd = vq->vdev->priv;
	struct virtio_video_msg *msg = (struct virtio_video_msg *)data;

	if (msg->hdr.type == SESSION_ERROR) {
		v4l2_err(&vvd->v4l2_dev,
			"session error in HAB command response\n");
	} else {
		spin_lock(&vvd->commandq.qlock);

		list_for_each_entry_safe(vbuf, vbuf_tmp,
			&vvd->pending_vbuf_list, pending_list_entry) {

			struct virtio_video_cmd_hdr* hdr
				= (struct virtio_video_cmd_hdr*)vbuf->buf;

			if (hdr->type == msg->hdr.type) {
				found = 1;
				memcpy(vbuf->resp_buf,
					(uint8_t*)msg + vbuf->size + vbuf->data_size,
					vbuf->resp_size);
				resp = (struct virtio_video_resp*)vbuf->resp_buf;

				v4l2_info(&vvd->v4l2_dev,
					"%s: got response from BE: cmd_type is: %s, "
					"response is %s\n", __func__, cmd_to_string(hdr->type),
					cmd_to_string(resp->result));

				vq_buf = kmalloc(sizeof(*vq_buf), GFP_KERNEL);
				vq_buf->buf = vbuf;
				list_add_tail(&vq_buf->list, &hvq->resp_list);
				break;
			}
		}

		spin_unlock(&vvd->commandq.qlock);
	}

	if (found)
		virtio_video_cmd_cb(vq);

	return 0;
}

static int process_msm_hab_evt_resp(struct hab_virtqueue* hvq, uint8_t* data)
{
	struct hab_vq_buffer *vq_buf = NULL;
	struct virtio_video_event *evt = NULL;
	struct virtqueue *vq = &hvq->vq;
	struct virtio_video_device *vvd = vq->vdev->priv;
	int size_bytes = MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE;
	int ret = 0;

	if (!(vq_buf = kmalloc(sizeof(*vq_buf), GFP_KERNEL))) {
		v4l2_err(&vvd->v4l2_dev, "%s alloc vq_buf failed\n", __func__);
		ret = -ENOMEM;
		goto err;
	}

	if (!(evt = kmalloc(size_bytes, GFP_KERNEL))) {
		v4l2_err(&vvd->v4l2_dev, "%s alloc evt buf failed\n", __func__);
		ret = -ENOMEM;
		goto err;
	}

	vq_buf->buf = evt;

	memcpy(evt, data, size_bytes);

	v4l2_info(&vvd->v4l2_dev, "%s: event received, event type %#x, stream id %d\n",
		__func__, evt->event_type, evt->stream_id);

	spin_lock(&hvq->qlock);
	list_add_tail(&vq_buf->list, &hvq->resp_list);
	spin_unlock(&hvq->qlock);

	virtio_video_event_cb(vq);

	return 0;
err:
	return ret;
}

static int virtio_video_hab_resp_handler(void* p)
{
	struct hab_virtqueue* hvq = p;
	uint8_t msg[MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE] = {0};
	uint32_t size_bytes = sizeof(msg);
	int ret = 0;

	while (!kthread_should_stop()) {
		ret = habmm_socket_recv(hvq->habmm_handle,
					(void *)msg, &size_bytes, 0, 0);

		if (unlikely(ret)) {
			if (-EINTR == ret) {
				continue;
			}
			else if (-ENODEV == ret) {
				pr_info("virtio-video: %s: socket 0x%x closed\n",
				        __func__, hvq->habmm_handle);
				goto exit;
			}
			else {
				pr_err("virtio-video: %s: socket recv failed: rc=%d\n",
				       __func__, ret);
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
	pr_err("virtio-video: %s: exited: error %d\n", __func__, ret);
	return ret;
}

static int start_resp_handler(struct hab_virtqueue *hvq)
{
	int ret = 0;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;

	hvq->resp_thread = kthread_run(virtio_video_hab_resp_handler,
				       (void *)hvq, "vvid_rsp_%s", hvq->vq.name);

	if (IS_ERR(hvq->resp_thread)) {
		v4l2_err(&vvd->v4l2_dev, "failed to create %s handler thread\n",
			 hvq->vq.name);
		ret = PTR_ERR(hvq->resp_thread);
	}

	return ret;
}

void* msm_hab_virtqueue_get_buf(struct virtqueue* vq, unsigned int* len)
{
	struct hab_vq_buffer* entry = NULL;
	struct hab_virtqueue * hvq = to_hab_vq(vq);
	void* buf = NULL;

	spin_lock(&hvq->qlock);
	entry = list_first_entry_or_null(&hvq->resp_list,
					 struct hab_vq_buffer, list);

	if (entry) {
		list_del(&entry->list);
		buf = entry->buf;
		kfree(entry);
	}
	spin_unlock(&hvq->qlock);

	return buf;
}

void* msm_hab_virtqueue_detach_unused_buf(struct virtqueue* vq)
{
	return NULL;
}

int msm_hab_virtqueue_add_sgs(struct virtqueue *vq, struct scatterlist *sgs[],
			      unsigned int out_sgs, unsigned int in_sgs, void *data, gfp_t gfp)
{
	return 0;
}

int msm_hab_virtqueue_add_inbuf(struct virtqueue *vq, struct scatterlist sg[],
				unsigned int num, void *data, gfp_t gfp)
{
	return 0;
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
		if(!(hvq = devm_kzalloc(&vdev->dev, sizeof(*hvq), GFP_KERNEL))) {
			ret = -ENOMEM;
			goto err;
		}

		hvq->vq.callback = callbacks[i];
		hvq->vq.name = names[i];
		hvq->vq.vdev = vdev;
		hvq->habmm_handle = 0;
		spin_lock_init(&hvq->qlock);
		INIT_LIST_HEAD(&hvq->vbuf_list);
		INIT_LIST_HEAD(&hvq->resp_list);

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

