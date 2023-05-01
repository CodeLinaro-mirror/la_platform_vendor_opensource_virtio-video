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

static int start_cmd_resp_handler(struct virtio_video_device* vvd);
static void stop_cmd_resp_handler(struct virtio_video_device* vvd);
static int start_event_handler(struct virtio_video_device* vvd);
static void stop_event_handler(struct virtio_video_device* vvd);

int virtio_video_msm_queue_cmd_buffer(struct virtio_video_device* vvd,
	struct virtio_video_vbuffer* vbuf)
{
	int ret = 0;
	uint32_t habmm_handle = vvd->commandq.vq->habmm_handle;
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
		__func__, habmm_handle, cmd_to_string(msg.hdr.type), msg.hdr.stream_id);

	ret = habmm_socket_send(habmm_handle, &msg, sizeof(msg), 0);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: habmm_socket_send failed %d\n", __func__,
			ret);
	}

	spin_unlock(&vvd->commandq.qlock);

	return ret;
}

int virtio_video_msm_hab_open(struct virtio_video_device* vvd)
{
	int ret = 0;
	uint32_t* ph = NULL;

	ph = &vvd->commandq.vq->habmm_handle;
	ret = habmm_socket_open(ph, MM_VID, 0, 0);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev,"habmm command socket open failed %d", ret);
		goto err;
	}

	ret = start_cmd_resp_handler(vvd);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev,"start response handler failed");
		goto err;
	}

	v4l2_info(&vvd->v4l2_dev, "commandq hab open done, handle %x", *ph);

	ph = &vvd->eventq.vq->habmm_handle;
	ret = habmm_socket_open(ph, MM_VID, 0, 0);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev,"habmm event socket open failed %d", ret);
		goto err;
	}

	ret = start_event_handler(vvd);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev,"start event handler failed %d", ret);
		goto err;
	}

	v4l2_info(&vvd->v4l2_dev, "eventq hab open done, handle %x", *ph);

err:
	return ret;
}

void virtio_video_msm_hab_close(struct virtio_video_device* vvd)
{
	int ret = 0;

	if (vvd->commandq.vq->habmm_handle) {
		stop_cmd_resp_handler(vvd);
		ret = habmm_socket_close(vvd->commandq.vq->habmm_handle);
		if (ret)
			v4l2_err(&vvd->v4l2_dev, "habmm command socket close failed %d", ret);
	}

	if (vvd->eventq.vq->habmm_handle) {
		stop_event_handler(vvd);
		ret = habmm_socket_close(vvd->eventq.vq->habmm_handle);
		if (ret)
			v4l2_err(&vvd->v4l2_dev, "habmm event socket close failed %d", ret);
	}
}

static void stop_cmd_resp_handler(struct virtio_video_device* vvd)
{
	struct hab_vq_buffer* entry = NULL;
	struct hab_vq_buffer* tmp = NULL;
	struct list_head resp_list;
	/** shut down the callback loop and free memory **/
	if (vvd->cmd_resp_thread) {
		vvd->exit_resp_handler = TRUE;
		vvd->cmd_resp_thread = 0;
	}
	resp_list = vvd->commandq.vq->resp_list;
	list_for_each_entry_safe(entry, tmp, &resp_list, list) {
		list_del(&entry->list);
		kfree(entry);
	}
}

static void stop_event_handler(struct virtio_video_device* vvd)
{
	if (vvd->evt_resp_thread) {
		vvd->exit_event_handler = TRUE;
		vvd->evt_resp_thread = 0;
	}
}

void process_msm_hab_cmd_resp(struct virtio_video_device* vvd,
	struct virtio_video_msg* msg)
{
	int found = 0;
	struct virtio_video_vbuffer* vbuf = NULL, * vbuf_tmp = NULL;
	struct virtio_video_resp* resp = NULL;
	struct hab_vq_buffer* vq_buf = NULL;

	if (msg->hdr.type == SESSION_ERROR) {
		v4l2_err(&vvd->v4l2_dev,
			"Detected session error in HAB command response");
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
				list_add_tail(&vq_buf->list, &vvd->commandq.vq->resp_list);
				break;
			}
		}

		spin_unlock(&vvd->commandq.qlock);
	}

	if (found)
		virtio_video_cmd_cb(vvd->commandq.vq);
}

void process_msm_hab_evt_resp(struct virtio_video_device* vvd,
	struct virtio_video_event* evt)
{
	struct hab_vq_buffer* vq_buf = NULL;

	vq_buf = kmalloc(sizeof(*vq_buf), GFP_KERNEL);
	vq_buf->buf = evt;

	v4l2_info(&vvd->v4l2_dev, "%s: event received, event type %#x, stream id %d",
		__func__, evt->event_type, evt->stream_id);

	spin_lock(&vvd->eventq.qlock);

	list_add_tail(&vq_buf->list, &vvd->eventq.vq->resp_list);

	spin_unlock(&vvd->eventq.qlock);

	virtio_video_event_cb(vvd->eventq.vq);
}


static int virtio_video_hab_cmd_resp_handler(void* p)
{
	struct virtio_video_device* vvd = (struct virtio_video_device*)p;
	struct virtio_video_msg msg = { 0 };
	uint32_t size_bytes = sizeof(msg);
	int ret = 0;
	uint32_t habmm_handle = vvd->commandq.vq->habmm_handle;
	int done = vvd->exit_resp_handler;

	while (!done) {

		memset(&msg, 0, size_bytes);
		ret = habmm_socket_recv(habmm_handle, (void*)&msg, &size_bytes, 0, 0);

		if (ret) {
			if (ret == -EINTR)
				v4l2_info(&vvd->v4l2_dev, "%s: receive error, retrying", __func__);
			else
				v4l2_err(&vvd->v4l2_dev, "%s: socket recv failed: hab ret code %d",
					 __func__, ret);
		} else {
			process_msm_hab_cmd_resp(vvd, &msg);
		}

		if (vvd->exit_resp_handler == true)
			done = 1;
	}

	v4l2_info(&vvd->v4l2_dev, "%s: exited: %d", __func__, ret);

	return ret;
}

static int virtio_video_hab_evt_resp_handler(void* p)
{
	struct virtio_video_device* vvd = (struct virtio_video_device*)p;
	struct virtio_video_event *evt = NULL;
	uint32_t size_bytes = sizeof(*evt);
	int ret = 0;
	int done = vvd->exit_event_handler;

	while (!done) {
		evt = kmalloc(size_bytes, GFP_KERNEL);
		ret = habmm_socket_recv(vvd->eventq.vq->habmm_handle,
					(void*)evt, &size_bytes, 0, 0);

		if (ret) {
			if (-EINTR == ret)
				v4l2_info(&vvd->v4l2_dev, "%s: receive error, retrying",
					__func__);
			else
				v4l2_err(&vvd->v4l2_dev, "%s: socket recv failed: "
					"hab ret code %d", __func__, ret);
		} else {
			process_msm_hab_evt_resp(vvd, evt);
		}

		if (vvd->exit_event_handler == true)
			done = 1;
	}

	v4l2_info(&vvd->v4l2_dev, "%s: exited: %d", __func__, ret);

	return ret;
}

static int start_cmd_resp_handler(struct virtio_video_device* vvd)
{
	int ret = 0;

	vvd->exit_resp_handler = false;
	vvd->cmd_resp_thread = kthread_run(virtio_video_hab_cmd_resp_handler,
					   (void*)vvd, "virtio_video_hab_cmd_resp_handler");

	if (IS_ERR(vvd->cmd_resp_thread)) {
		v4l2_err(&vvd->v4l2_dev, "failed to create response handler thread");
		ret = -1;
	}

	return ret;
}

static int start_event_handler(struct virtio_video_device* vvd)
{
	int ret = 0;

	vvd->exit_event_handler = false;
	vvd->evt_resp_thread = kthread_run(virtio_video_hab_evt_resp_handler,
					   (void*)vvd, "virtio_video_hab_evt_resp_handler");

	if (IS_ERR(vvd->evt_resp_thread)) {
		v4l2_err(&vvd->v4l2_dev, "failed to create event handler thread");
		ret = -1;
	}

	return ret;
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

int msm_hab_virtqueue_add_sgs(struct msm_hab_virtqueue *vq, struct scatterlist *sgs[],
			      unsigned int out_sgs, unsigned int in_sgs, void *data, gfp_t gfp)
{
	return 0;
}

int msm_hab_virtqueue_add_inbuf(struct msm_hab_virtqueue *vq, struct scatterlist sg[],
				unsigned int num, void *data, gfp_t gfp)
{
	return 0;
}
