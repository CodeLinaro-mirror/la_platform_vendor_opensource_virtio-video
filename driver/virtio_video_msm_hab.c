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

#define SESSION_ERROR -1
#define DEFAULT_VQ_NUM 512

#define HVQ_NUM 2
#define COMMANDQ 0
#define EVENTQ 1
#define FEATURE_BYTES 8

#define MAX_RETRY_FOR_GET_EVT_BUF 100

#define HAB_TIMEOUT 1000

struct virtio_video_initial_data {
	uint64_t features;
	struct virtio_video_config configs;
	uint8_t padding[MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE -
			sizeof(uint64_t) - sizeof(struct virtio_video_config)];
};

static struct hab_virtqueue hvq_dec[HVQ_NUM];
static struct hab_virtqueue hvq_enc[HVQ_NUM];
static struct virtio_video_initial_data dec_data;
static struct virtio_video_initial_data enc_data;

static int start_resp_handler(struct hab_virtqueue *hvq);
static void stop_cmd_resp_handler(struct hab_virtqueue* hvq);
static void stop_event_handler(struct hab_virtqueue* hvq);

#define hvq_event_lock() \
{\
	if (hvq->type == MSM_VIRTQ_EVT_TYPE) \
		spin_lock(&hvq->qlock); \
}

#define hvq_event_unlock() \
{\
	if (hvq->type == MSM_VIRTQ_EVT_TYPE) \
		spin_unlock(&hvq->qlock); \
}

bool msm_hab_virtqueue_is_broken(struct virtqueue *vq)
{
	struct hab_virtqueue * hvq = to_hab_vq(vq);

	return READ_ONCE(hvq->broken);
};

static inline void free_hvq_unused_vq_buf_list(struct hab_virtqueue *hvq)
{
	struct hab_vq_buffer *vq_buf = NULL, *tmp = NULL;

	list_for_each_entry_safe(vq_buf, tmp,
	                         &hvq->unused_vq_buf_list.list, list) {
		list_del(&vq_buf->list);
		hvq->unused_vq_buf_list.count--;
		kfree(vq_buf);
	}
}

static int init_hvq_unused_vq_buf_list(struct virtio_video_device *vvd,
                                       struct hab_virtqueue *hvq)
{
	struct hab_vq_buffer *vq_buf = NULL;
	int ret = 0;
	int i = 0;

	INIT_LIST_HEAD(&hvq->unused_vq_buf_list.list);
	hvq->unused_vq_buf_list.count = 0;

	for (i = 0; i < hvq->vq.num_free; i++) {
		if (!(vq_buf = kzalloc(sizeof(*vq_buf), GFP_KERNEL))) {
			vpr_e(vvd2tag(vvd), "%s %s out of memory\n",
			      __func__, hvq->vq.name);
			free_hvq_unused_vq_buf_list(hvq);
			ret = -ENOMEM;
			break;
		}

		list_add_tail(&vq_buf->list, &hvq->unused_vq_buf_list.list);
		hvq->unused_vq_buf_list.count++;
	}

	return ret;
}

static int attach_buf_to_vq_buf(struct hab_virtqueue *hvq,
                                struct hab_list *ls, void *data)
{
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct hab_vq_buffer *vq_buf = NULL;
	int ret = 0;

	hvq_event_lock();
	vq_buf = list_first_entry_or_null(&hvq->unused_vq_buf_list.list,
	                                  struct hab_vq_buffer, list);
	if (vq_buf) {
		list_del(&vq_buf->list);
		hvq->unused_vq_buf_list.count--;

		vq_buf->buf = data;

		list_add_tail(&vq_buf->list, &ls->list);
		ls->count++;
	} else {
		vpr_e(vvd2tag(vvd), "%s: no available vq_buf\n", __func__);
		ret = -ENOENT;
	}

	hvq_event_unlock();

	return ret;
}

static void* unattach_buf_from_vq_buf(struct hab_virtqueue *hvq,
                                      struct hab_list *ls)
{
	struct hab_vq_buffer *vq_buf = NULL;
	void *data = NULL;

	hvq_event_lock();

	vq_buf = list_first_entry_or_null(&ls->list, struct hab_vq_buffer, list);

	if (vq_buf) {
		list_del(&vq_buf->list);
		ls->count--;

		list_add_tail(&vq_buf->list, &hvq->unused_vq_buf_list.list);
		hvq->unused_vq_buf_list.count++;
		data = vq_buf->buf;
		vq_buf->buf = NULL;
	}

	hvq_event_unlock();

	return data;
}

static int virtio_video_msm_hab_open(struct virtio_device *vdev, struct hab_virtqueue *hvq)
{
	int ret = 0;
	int mmid = MM_VID;
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: mmid=%d\n", __func__, mmid);

	ret = habmm_socket_open(&hvq->habmm_handle, mmid, HAB_TIMEOUT, 0);
	if (ret) {
		vpr_e(vvd2tag(vvd), "%s: socket open failed %d\n", __func__, ret);
		goto err;
	}

	ret = start_resp_handler(hvq);
	if (ret) {
		goto err_start_handler;
	}

	vpr_h(vvd2tag(vvd), "%s: mmid=%d done, socket 0x%x\n", __func__, mmid, hvq->habmm_handle);
	WRITE_ONCE(hvq->broken, false);

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
			vpr_e(vvd2tag(vvd), "%s, habmm cmd socket close failed %d\n",
			      __func__, ret);
		stop_cmd_resp_handler(hvq);
	}

	if (hvq->type == MSM_VIRTQ_EVT_TYPE && hvq->habmm_handle) {
		ret = habmm_socket_close(hvq->habmm_handle);
		if (ret)
			vpr_e(vvd2tag(vvd), "%s: habmm event socket close failed %d\n",
			      __func__, ret);
		stop_event_handler(hvq);
	}

	vpr_h(vvd2tag(vvd), "%s: %s socket=0x%x done\n", __func__,
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

	list_for_each_entry_safe(entry, tmp, &hvq->resp_list.list, list) {
		list_del(&entry->list);
		hvq->resp_list.count--;
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
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct virtio_video_cmd_hdr *hdr = (struct virtio_video_cmd_hdr *)data;
	struct virtio_video_cmd_hdr *vbuf_hdr = NULL;
	void *resp = NULL;
	int ret = 0;
	int resp_rc = 0;

	if (hdr->type == SESSION_ERROR) {
		vpr_e(vvd2tag(vvd), "%s: session error stream_id %d\n",
		      __func__, hdr->stream_id);
		ret = -ENODEV;
		goto err;
	}

	spin_lock(&vvd->commandq.qlock);

	list_for_each_entry_safe(vbuf, vbuf_tmp,
				&vvd->pending_vbuf_list, pending_list_entry) {

		vbuf_hdr = (struct virtio_video_cmd_hdr*)vbuf->buf;

		if (vbuf_hdr->stream_id == hdr->stream_id &&
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

		vpr_h(stream_id2tag(vvd, hdr->stream_id), "%s: recv: cmd_type=%s. resp=%s\n",
		      __func__,
		      cmd_to_string(hdr->type),
		      cmd_to_string(resp_rc));

		attach_buf_to_vq_buf(hvq, &hvq->resp_list, vbuf);
	}
	else {
		vpr_e(vvd2tag(vvd), "%s: unable find pending vbuf stream_id=%d\n",
		      __func__, hdr->stream_id);
		ret = -EINVAL;
	}

	spin_unlock(&vvd->commandq.qlock);

	if (found)
		virtio_video_cmd_cb(vq);

err:
	return ret;
}

static int process_msm_hab_evt_resp(struct hab_virtqueue *hvq, void *data)
{
	struct virtio_video_msm_event *evt = (struct virtio_video_msm_event *)data;
	struct virtqueue *vq = &hvq->vq;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	char *tag = stream_id2tag(vvd, evt->stream_id);
	int ret = 0;

	vpr_h(tag, "%s: recv: %s, stream_id %d, resp_list cnt %d\n",
	      __func__, event2str(evt->event_type), evt->stream_id,
	      hvq->resp_list.count);

	ret = attach_buf_to_vq_buf(hvq, &hvq->resp_list, evt);

	if (!ret)
		virtio_video_event_cb(vq);

	return ret;
}

static void *get_one_evt_buf(struct hab_virtqueue *hvq, const int max_retry)
{
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	void *msg = NULL;
	int retry = 0;

	while (list_empty(&hvq->vbuf_list.list) && retry > max_retry) {
		vpr_e(vvd2tag(vvd), "%s: no avail event buf, retry %d\n",
		      __func__, retry);
		usleep_range(20, 100);
		retry++;
		trace_hab_resp_evt_dq_vqbuf_err(hvq->vbuf_list.count);
	}
	msg = unattach_buf_from_vq_buf(hvq, &hvq->vbuf_list);

	return msg;
}

static int virtio_video_send_err_evt(struct hab_virtqueue *hvq, int stream_id)
{
	int ret = 0;
	const char *vq_name = hvq->vq.name;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct virtio_video_msm_event *evt = NULL;

	evt = get_one_evt_buf(hvq, 100 * MAX_RETRY_FOR_GET_EVT_BUF);
	if (evt) {
		vpr_h(vvd2tag(vvd), "%s %s: to send error event, resp_list=%d\n",
		      vq_name, __func__, hvq->resp_list.count);
		evt->event_type = VIRTIO_VIDEO_EVENT_ERROR;
		evt->stream_id = stream_id;
		ret = process_msm_hab_evt_resp(hvq, evt);
	} else {
		vpr_e(vvd2tag(vvd), "%s %s: unable get event buf\n", vq_name, __func__);
		ret = -ENOENT;
	}

	return ret;
}

static int virtio_video_hab_resp_handler(void *p)
{
	struct hab_virtqueue *hvq = p;
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct virtio_video_stream *stream = NULL;
	const char *vq_name = hvq->vq.name;
	uint8_t buf[MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE] = {0};
	int size_bytes = 0;
	void *msg = NULL;
	int ret = 0;
	int stream_id = 0;

	vpr_h(vvd2tag(vvd), "%s %s: start\n", vq_name, __func__);

	while (!kthread_should_stop()) {
		msg = NULL;

		if (hvq->type == MSM_VIRTQ_EVT_TYPE) {
			vpr_l(vvd2tag(vvd), "%s %s: wait to recv, vbuf_list=%d\n",
			      vq_name, __func__, hvq->vbuf_list.count);
			trace_hab_resp_evt_dq_vqbuf(hvq->vbuf_list.count);
			msg = get_one_evt_buf(hvq, MAX_RETRY_FOR_GET_EVT_BUF);
			if (unlikely(!msg)) {
				vpr_e(vvd2tag(vvd), "%s %s: unable get event buf\n",
				      vq_name, __func__);
				ret = -ENOENT;
				goto err;
			}
			size_bytes = sizeof(struct virtio_video_msm_event);
		} else {
			vpr_l(vvd2tag(vvd), "%s %s: wait to recv\n", vq_name, __func__);
			trace_hab_resp_cmd_dq_vqbuf(1);
			msg = buf;
			size_bytes = sizeof(buf);
		}

		memset(msg, 0, size_bytes);
		ret = habmm_socket_recv(hvq->habmm_handle,
		                        msg, &size_bytes, 0, 0);
		if (unlikely(ret)) {
			if (ret == -EINTR) {
				if (hvq->type == MSM_VIRTQ_EVT_TYPE)
					attach_buf_to_vq_buf(hvq, &hvq->vbuf_list, msg);
				continue;
			}
			else if (ret == -ENODEV) {
				vpr_e(vvd2tag(vvd), "%s %s: socket 0x%x closed, recv failed\n",
				      vq_name, __func__, hvq->habmm_handle);
			}
			else {
				vpr_e(vvd2tag(vvd), "%s %s: socket recv failed: rc=%d\n",
				      vq_name, __func__, ret);
			}
			WRITE_ONCE(hvq->broken, true);
			goto exit;
		}

		if (hvq->type == MSM_VIRTQ_CMD_TYPE) {
			vpr_l(vvd2tag(vvd), "%s %s: recv done\n", vq_name, __func__);
			ret = process_msm_hab_cmd_resp(hvq, msg);
			trace_hab_resp_cmd_done(hvq->resp_list.count);
			vpr_h(vvd2tag(vvd), "%s %s: process done\n", vq_name, __func__);
		}
		else {
			vpr_l(vvd2tag(vvd), "%s %s: recv done, resp_list=%d\n",
			      vq_name, __func__, hvq->resp_list.count);

			ret = process_msm_hab_evt_resp(hvq, msg);

			trace_hab_resp_evt_done(hvq->resp_list.count);
			vpr_h(vvd2tag(vvd), "%s %s: process done, resp_list=%d\n",
			      vq_name, __func__, hvq->resp_list.count);
		}

err:
		if (ret && hvq->type == MSM_VIRTQ_EVT_TYPE) {
			stream_id = ((struct virtio_video_msm_event *)msg)->stream_id;
			virtio_video_send_err_evt(hvq, stream_id);
		}
	}

	return ret;

exit:
	vpr_e(vvd2tag(vvd), "%s %s: exited. error %d\n", vq_name, __func__, ret);

	//for commandq, commands will wait until timeout; no specific handling here
	//for eventq, sending out error event
	if (hvq->type == MSM_VIRTQ_EVT_TYPE) {
		if (msg)
			attach_buf_to_vq_buf(hvq, &hvq->vbuf_list, msg);

		idr_for_each_entry(&vvd->stream_idr, stream, stream_id)
			virtio_video_send_err_evt(hvq, stream_id);
	}

	if (vvd->vq_running) {
		ret = habmm_socket_close(hvq->habmm_handle);
		if (ret)
			vpr_e(vvd2tag(vvd), "%s, habmm cmd socket close failed %d\n",
			      __func__, ret);
		else
			hvq->habmm_handle = 0;

		if (vvd->type == VIRTIO_VIDEO_DEVICE_DECODER &&
		    hvq->type == MSM_VIRTQ_CMD_TYPE) {
			schedule_delayed_work(&vvd->connect_work,
					      msecs_to_jiffies(CONNECT_DELAY));
			vpr_h(vvd2tag(vvd), "%s %s: scheduled reconnect\n",
			      vq_name, __func__);
		}
	}

	return ret;
}

static int start_resp_handler(struct hab_virtqueue *hvq)
{
	int ret = 0;
	struct virtio_video_device *vvd = NULL;
	hvq->resp_thread = kthread_create(virtio_video_hab_resp_handler,
	                                  hvq, "vvid_rsp_%x", hvq);

	if (IS_ERR(hvq->resp_thread)) {
		vpr_e(vvd2tag(vvd), "%s: failed\n", __func__);
		ret = PTR_ERR(hvq->resp_thread);
	}

	return ret;
}

static int get_hab_handle(struct virtio_device *vdev, struct hab_virtqueue hvq[])
{
	int ret = 0;
	int i = 0;
	struct virtio_video_device *vvd = vdev->priv;

	for (i = 0; i < HVQ_NUM; i++) {
		ret = virtio_video_msm_hab_open(vdev, &hvq[i]);
		if (ret) {
			vpr_e(vvd2tag(vvd), "failed to get hab handle");
			break;
		}
	}

	return ret;
}

#ifndef MSM_VIDC_HW_VIRT
static int get_inital_data(struct hab_virtqueue hvq[], struct virtio_video_initial_data* data)
{
	int size_bytes = MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE;
	int ret = 0;
	struct virtio_video_device *vvd = NULL;

	ret = habmm_socket_recv(hvq[EVENTQ].habmm_handle, data, &size_bytes,
				HAB_TIMEOUT, HABMM_SOCKET_RECV_FLAGS_TIMEOUT);
	if (ret)
		vpr_e(vvd2tag(vvd), "failed to receive data, ret %d", ret);

	return ret;
}
#endif

int msm_hab_vdev_init(struct virtio_device *vdev)
{
	struct hab_virtqueue *hvq = NULL;
	struct virtio_video_initial_data *data = NULL;
	int ret = 0;

	if (vdev->id.device == VIRTIO_ID_VIDEO_DECODER) {
		hvq = hvq_dec;
		data = &dec_data;
	} else {
		hvq = hvq_enc;
		data = &enc_data;
	}

	ret = get_hab_handle(vdev, hvq);
	if (ret)
		goto err;

#ifndef MSM_VIDC_HW_VIRT
	ret = get_inital_data(hvq, data);
#endif

err:
	return ret;
}

uint64_t msm_hab_get_features(struct virtio_device *vdev)
{
	uint64_t features = 0;
	struct virtio_video_device *vvd = vdev->priv;

	if (vdev->id.device == VIRTIO_ID_VIDEO_DECODER)
		features = dec_data.features;
	else
		features = enc_data.features;

	vpr_h(vvd2tag(vvd), "%s: video%d, features %#x", __func__, vdev->id.device, features);

	return features;
}

int msm_hab_set_features(struct virtio_device *vdev)
{
	struct hab_virtqueue *hvq = NULL;
	uint32_t size_bytes = MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE;
	struct virtio_video_initial_data *data = NULL;
	struct virtio_video_device *vvd = vdev->priv;
	int ret = 0;

	if (vdev->id.device == VIRTIO_ID_VIDEO_DECODER) {
		hvq = hvq_dec;
		data = &dec_data;
	} else {
		hvq = hvq_enc;
		data = &enc_data;
	}

	ret = habmm_socket_send(hvq[EVENTQ].habmm_handle, data, size_bytes, 0);
	if (ret)
		vpr_e(vvd2tag(vvd), "failed to send features to BE");

	return ret;
}

struct virtio_video_config msm_hab_get_config(struct virtio_device *vdev)
{
	return (vdev->id.device == VIRTIO_ID_VIDEO_DECODER) ?
		dec_data.configs : enc_data.configs;
}

bool msm_hab_virtqueue_kick(struct virtqueue *vq)
{
	struct hab_virtqueue *hvq = to_hab_vq(vq);
	struct virtio_video_device *vvd = hvq->vq.vdev->priv;
	struct virtio_video_stream *stream = NULL;
	uint32_t habmm_handle = hvq->habmm_handle;
	struct virtio_video_msg *msg = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	unsigned long flags = 0L;
	char *tag = vvd2tag(vvd);
	int stream_id = 0;
	int ret = 0;

	if (hvq->type == MSM_VIRTQ_CMD_TYPE) {
		if (unlikely(READ_ONCE(hvq->broken)))
			goto err;

		vbuf = unattach_buf_from_vq_buf(hvq, &hvq->vbuf_list);

		if (vbuf == NULL)
			goto err;

		spin_unlock_irqrestore(&vvd->commandq.qlock, flags);

		msg = (struct virtio_video_msg *)vbuf->buf;
		stream_id = msg->hdr.stream_id;
		stream = idr_find(&vvd->stream_idr, stream_id);
		tag = strm2tag(stream);

		vpr_h(tag, "%s: socket 0x%x: type %s, stream_id %d, %d %d %d\n",
		      __func__, habmm_handle,
		      cmd_to_string(msg->hdr.type),
		      msg->hdr.stream_id, vbuf->size,
		      vbuf->data_size, vbuf->resp_size);

		ret = habmm_socket_send(habmm_handle, msg, sizeof(*msg), 0);
		if (ret) {
			WRITE_ONCE(hvq->broken, true);

			vpr_e(tag, "%s: habmm_socket_send failed %d\n",
			      __func__, ret);
		}

		spin_lock_irqsave(&vvd->commandq.qlock, flags);

		if (ret)
			goto err;
	}

	return true;

err:
	vpr_e(tag, "%s: vq broken, skip", __func__);
	return false;
}

void msm_hab_sg_init_one(struct scatterlist *sg, const void *buf,
                         unsigned int buflen)
{
}

void* msm_hab_virtqueue_get_buf(struct virtqueue* vq, unsigned int* len)
{
	struct hab_virtqueue *hvq = to_hab_vq(vq);
	struct virtio_video_device *vvd = vq->vdev->priv;
	void *buf = NULL;

	buf = unattach_buf_from_vq_buf(hvq, &hvq->resp_list);

	vpr_h(vvd2tag(vvd), "%s %s, resp_list count=%d\n",
	      vq->name, __func__, hvq->resp_list.count);
	return buf;
}

void* msm_hab_virtqueue_detach_unused_buf(struct virtqueue* vq)
{
	struct hab_virtqueue *hvq = to_hab_vq(vq);
	void *buf = NULL;

	buf = unattach_buf_from_vq_buf(hvq, &hvq->vbuf_list);

	if (!buf)
		buf = unattach_buf_from_vq_buf(hvq, &hvq->resp_list);

	if (!buf) {
		hvq_event_lock();
		free_hvq_unused_vq_buf_list(hvq);
		hvq_event_unlock();
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
	struct virtio_video_device *vvd = vq->vdev->priv;

	if (unlikely(READ_ONCE(hvq->broken))) {
		return -EIO;
	}
	hvq_event_lock();
	vq_buf = list_first_entry_or_null(&hvq->unused_vq_buf_list.list,
	                                  struct hab_vq_buffer, list);

	if (vq_buf) {
		list_del(&vq_buf->list);
		hvq->unused_vq_buf_list.count--;
	}
	hvq_event_unlock();

	if (!vq_buf) {
		vpr_e(vvd2tag(vvd), "%s: %s %s out of memory\n",
		      name, __func__, hvq->vq.name);
		goto err;
	}

	vq_buf->buf = data;

	hvq_event_lock();
	list_add_tail(&vq_buf->list, &hvq->vbuf_list.list);
	hvq->vbuf_list.count++;
	hvq_event_unlock();

	vpr_l(vvd2tag(vvd), "%s %s, vbuf_list count=%d\n",
	      vq->name, __func__, hvq->vbuf_list.count);

	if (hvq->type == MSM_VIRTQ_EVT_TYPE)
		trace_hab_evt_add_vqbuf(hvq->vbuf_list.count);
	else
		trace_hab_cmd_add_vqbuf(hvq->vbuf_list.count);

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
	struct virtio_video_device *vvd = vdev->priv;
	int i = 0;

	vpr_h(vvd2tag(vvd), "%s: %s\n", dev_name(&vdev->dev), __func__);

	for (i = 0; i < nvqs; i++) {
		if (vdev->id.device == VIRTIO_ID_VIDEO_DECODER)
			hvq = &hvq_dec[i];
		else
			hvq = &hvq_enc[i];

		if (!hvq->habmm_handle) {
			vpr_e(vvd2tag(vvd), "%s: %s habmm_handle is null\n",
			      dev_name(&vdev->dev), __func__);
			ret = -ENODEV;
			goto err;
		}

		hvq->vq.callback = callbacks[i];
		hvq->vq.name = names[i];
		hvq->vq.vdev = vdev;
		hvq->vq.num_free = DEFAULT_VQ_NUM;
		INIT_LIST_HEAD(&hvq->vbuf_list.list);
		INIT_LIST_HEAD(&hvq->resp_list.list);
		if ((ret = init_hvq_unused_vq_buf_list(vvd, hvq)))
			goto err;
		hvq->vbuf_list.count = 0;
		hvq->resp_list.count = 0;

		if (!strcmp(names[i], "commandq"))
			hvq->type = MSM_VIRTQ_CMD_TYPE;
		else {
			hvq->type = MSM_VIRTQ_EVT_TYPE;
			spin_lock_init(&hvq->qlock);
		}
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
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: %s\n", dev_name(&vdev->dev), __func__);

	vvd->vq_running = false;
	list_for_each_entry_safe(vq, n, &vdev->vqs, list) {
		hvq = to_hab_vq(vq);
		virtio_video_msm_hab_close(hvq);
		list_del(&vq->list);
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