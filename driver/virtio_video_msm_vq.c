/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/errno.h>
#include "virtio_video.h"
#include "virtio_video_msm_debug.h"
#include "virtio_video_msm_hab.h"
#include <linux/videodev2.h>

static int virtio_video_v4l2_to_hab(struct virtio_video_device* vvd,
                                    struct virtio_video_stream* stream,
                                    enum virtio_video_cmd_type cmd_type,
                                    enum virtio_video_sub_cmd_type sub_cmd_type,
                                    void* payload, size_t size,
                                    void* priv, bool sync)
{
	int ret = 0;
	struct virtio_video_stream_ioctl_cmd* req_p;
	struct virtio_video_vbuffer* vbuf;
	size_t req_size = sizeof(req_p->hdr) + size;
	size_t resp_size = sync ? size : 0;

	req_p = virtio_video_alloc_req_resp(vvd, NULL, &vbuf, req_size,
					    resp_size, payload);

	if (IS_ERR(req_p)) {
		ret = -ENOMEM;
		goto err;
	}

	req_p->hdr.cmd_type = cmd_type;
	req_p->hdr.stream_id = stream->stream_id;
	req_p->hdr.sub_cmd_type = sub_cmd_type;

	memcpy(req_p->payload, payload, size);

	//Set vbuf.data_size to 0 for this moment
	vbuf->data_size = 0;
	vbuf->priv = priv;

	if (sync)
		ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
	else
		ret = virtio_video_queue_cmd_buffer(vvd, vbuf);

	if (ret) {
		virtio_video_state_update(stream, STREAM_STATE_ERROR);
		vpr_e(strm2tag(stream), "%s: %s cmd failed. %s-%s ret %d",
		      __func__, sync ? "sync" : "async",
		      cmd_to_string(cmd_type),
		      cmd_to_string(sub_cmd_type), ret);
	} else {
		vpr_h(strm2tag(stream), "%s: %s cmd done: %s-%s\n",
		      __func__, sync? "sync" : "async",
		      cmd_to_string(cmd_type),
		      cmd_to_string(sub_cmd_type));
	}

err:
	return ret;
}

static int virtio_video_v4l2_to_hab_async(struct virtio_video_device* vvd,
					  struct virtio_video_stream* stream,
					  enum virtio_video_cmd_type cmd_type,
					  enum virtio_video_sub_cmd_type sub_cmd_type,
					  void* payload, size_t size, void* priv)
{

	return virtio_video_v4l2_to_hab(vvd, stream, cmd_type, sub_cmd_type,
					payload, size, priv, false);
}

static int virtio_video_v4l2_to_hab_sync(struct virtio_video_device* vvd,
					 struct virtio_video_stream* stream,
					 enum virtio_video_cmd_type cmd_type,
					 enum virtio_video_sub_cmd_type sub_cmd_type,
					 void* payload, size_t size, void* priv)
{
	return virtio_video_v4l2_to_hab(vvd, stream, cmd_type, sub_cmd_type,
					payload, size, priv, true);
}

int virtio_video_cmd_enum_fmt(struct virtio_video_device* vvd,
			      struct virtio_video_stream* stream,
			      struct v4l2_fmtdesc* fmtdesc)
{
	int ret = 0;

	ret = virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, ENUM_FMT,
					     (void *)fmtdesc, sizeof(*fmtdesc), NULL);

    if (!ret && !fmtdesc->pixelformat)
		ret = -EINVAL;

	return ret;
}

int virtio_video_cmd_enum_frameintervals(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_frmivalenum* fival)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, ENUM_FRAMEINTERVALS,
					     (void *)fival, sizeof(*fival), NULL);
}

int virtio_video_cmd_enum_framesizes(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_frmsizeenum* fsize)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, ENUM_FRAMESIZES,
					     (void *)fsize, sizeof(*fsize), NULL);
}

int virtio_video_cmd_g_fmt(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_format* format)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, G_FMT,
					     (void *)format, sizeof(*format), NULL);
}

int virtio_video_cmd_s_fmt(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_format* format)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, S_FMT,
					     (void *)format, sizeof(*format), NULL);
}

int virtio_video_cmd_g_parm(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_streamparm* parm)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, G_PARAM,
					     (void *)parm, sizeof(*parm), NULL);
}

int virtio_video_cmd_s_parm(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_streamparm* parm)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, S_PARAM,
					     (void *)parm, sizeof(*parm), NULL);
}

int virtio_video_cmd_g_ctrl(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_control* control)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_CONTROL, G_CTRL,
					     (void *)control, sizeof(*control), NULL);
}

int virtio_video_cmd_s_ctrl(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_control* control)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_SET_CONTROL, S_CTRL,
					     (void *)control, sizeof(*control), NULL);
}

int virtio_video_cmd_g_selection(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_selection* sel)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, G_SELECTION,
					     (void *)sel, sizeof(*sel), NULL);
}

int virtio_video_cmd_s_selection(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_selection* sel)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, S_SELECTION,
					     (void *)sel, sizeof(*sel), NULL);
}

int virtio_video_cmd_qbuf(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_buffer* buf, void *priv)
{
	int ret = 0;
	int size = 0;

	if (V4L2_TYPE_IS_MULTIPLANAR(buf->type))
		size = sizeof(*buf) + buf->length * sizeof(buf->m.planes[0]);
	else
		size = sizeof(*buf);

	ret = virtio_video_v4l2_to_hab_async(vvd, stream,
					     VIRTIO_VIDEO_CMD_RESOURCE_QUEUE, QBUF,
					     (void *)buf, size, priv);

	return ret;
}

int virtio_video_cmd_querybuf(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_buffer* buf)
{
	vpr_e(strm2tag(stream), "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}

int virtio_video_cmd_querycap(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_capability* cap)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
	                     VIRTIO_VIDEO_CMD_GET_PARAMS, QUERYCAP,
	                     (void *)cap, sizeof(*cap), NULL);
}

int virtio_video_cmd_reqbufs(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_requestbuffers* buf)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, REQBUFS,
					     (void *)buf, sizeof(*buf), NULL);
}

int virtio_video_cmd_subscribe_event(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, const struct v4l2_event_subscription* sub)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, SUBSCRIBE_EVENT,
					     (void *)sub, sizeof(*sub), NULL);
}

int virtio_video_cmd_unsubscribe_event(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, const struct v4l2_event_subscription* sub)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, UNSUBSCRIBE_EVENT,
					     (void *)sub, sizeof(*sub), NULL);
}

int virtio_video_cmd_streamon(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, enum v4l2_buf_type type)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_STREAMON, 0,
					     (void *)&type, sizeof(type), NULL);
}

int virtio_video_cmd_streamoff(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, enum v4l2_buf_type type)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream,
					     VIRTIO_VIDEO_CMD_STREAMOFF, 0,
					     (void *)&type, sizeof(type), NULL);
}

int virtio_video_cmd_decoder_cmd(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_decoder_cmd* dec)
{
	enum virtio_video_cmd_type cmd_type;
	int ret = 0;

	if (dec->cmd == V4L2_DEC_CMD_START) {
		cmd_type = VIRTIO_VIDEO_CMD_STREAM_START;
	} else if (dec->cmd == V4L2_DEC_CMD_STOP) {
		cmd_type = VIRTIO_VIDEO_CMD_STREAM_DRAIN;
	} else {
		vpr_e(strm2tag(stream), "%s: invalid decoder command: %d\n", __func__, dec->cmd);
		ret = -EINVAL;
		goto err;
	}

	ret = virtio_video_v4l2_to_hab_sync(vvd, stream, cmd_type, 0, NULL, 0, NULL);

err:
	return ret;
}

int virtio_video_cmd_encoder_cmd(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_encoder_cmd* enc)
{
	enum virtio_video_cmd_type cmd_type;
	int ret = 0;

	if (enc->cmd == V4L2_ENC_CMD_START) {
		cmd_type = VIRTIO_VIDEO_CMD_STREAM_START;
	} else if (enc->cmd == V4L2_ENC_CMD_STOP) {
		cmd_type = VIRTIO_VIDEO_CMD_STREAM_DRAIN;
	} else {
		vpr_e(strm2tag(stream), "%s: invalid encoder command: %d\n", __func__, enc->cmd);
		ret = -EINVAL;
		goto err;
	}

	ret = virtio_video_v4l2_to_hab_sync(vvd, stream,
					    cmd_type, 0, NULL, 0, NULL);

err:
	return ret;
}

int virtio_video_cmd_try_decoder_cmd(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     struct v4l2_decoder_cmd* dec)
{
	vpr_e(strm2tag(stream), "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}

int virtio_video_cmd_try_encoder_cmd(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     struct v4l2_encoder_cmd* enc)
{
	vpr_e(strm2tag(stream), "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}

int virtio_video_cmd_try_fmt(struct virtio_video_device* vvd,
			     struct virtio_video_stream* stream,
			     struct v4l2_format* format)
{
	vpr_e(strm2tag(stream), "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}

#ifdef MSM_VIDC_HW_VIRT
static int msm_create_hw_virt_stream(struct virtio_video_device *vvd)
{
	int ret = 0;
	uint32_t stream_id = 0;
	struct virtio_video_stream *stream = NULL;
	char name[TASK_COMM_LEN]= {0};
	enum virtio_video_format format = VIRTIO_VIDEO_FORMAT_H264;

	stream = kzalloc(sizeof(*stream), GFP_KERNEL);
	if (!stream) {
		ret = -ENOMEM;
	} else {
		get_task_comm(name, current);
		stream->client_id = INVALID_CLIENT_ID;
		stream->codec = INVALID_CODEC;
		stream->domain = cpu_to_le32(vvd->type);
		snprintf(stream->tag, sizeof(stream->tag), VPR_TAG);
		virtio_video_stream_id_get(vvd, stream, &stream_id);
		stream->stream_id = stream_id;
		vvd->gvm_stream_id = stream_id;
		virtio_video_state_reset(stream);
		ret = virtio_video_cmd_stream_create(vvd, stream_id, format, name);
		if (ret) {
			vpr_e(strm2tag(stream), "failed to create hw_virt stream\n");
			virtio_video_stream_id_put(vvd, stream_id);
			kfree(stream);
		}
	}

	return ret;
}

static void
virtio_video_cmd_open_gvm_cb(struct virtio_video_device *vvd,
			     struct virtio_video_vbuffer *vbuf)
{
	struct virtio_video_open_gvm_resp *resp =
		(struct virtio_video_open_gvm_resp *)vbuf->resp_buf;

	vvd->device_core_mask = le32_to_cpu(resp->device_core_mask);
}

int32_t virtio_video_cmd_open_gvm(uint32_t vm_id, uint32_t device_id_mask)
{
	int32_t ret = 0;
	struct virtio_video_open_gvm *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	size_t resp_size = sizeof(struct virtio_video_open_gvm_resp);
	struct virtio_video_device *vvd = msm_virtio_video_hw_virtualization_get_vvd();

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		ret = -EXDEV;
	} else {
		ret = msm_create_hw_virt_stream(vvd);
		if (ret) {
			vpr_e(VPR_TAG, "%S: failed creating hw_virt stream", __func__);
		} else {
			req_p = virtio_video_alloc_req_resp(vvd,
			                                    virtio_video_cmd_open_gvm_cb,
			                                    &vbuf,
			                                    sizeof(*req_p),
			                                    resp_size,
			                                    NULL);
			req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_OPEN_GVM);
			req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
			req_p->vm_id = cpu_to_le32(vm_id);
			req_p->device_id_mask = cpu_to_le32(device_id_mask);
			ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
			if (ret == -ETIMEDOUT) {
			    vpr_e(vvd2tag(vvd), "timed out waiting for open gvm\n");
			} else if (vvd->device_id_mask == req_p->device_id_mask) {
			    vpr_e(vvd2tag(vvd), "open GVM BE returns NULL\n");
			    ret = -EINVAL;
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_cmd_open_gvm);

int32_t virtio_video_cmd_close_gvm(void)
{
	int32_t ret = 0;
	struct virtio_video_close_gvm *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virtualization_get_vvd();

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		ret = -EXDEV;
	} else {
		struct virtio_video_stream *stream = idr_find(&vvd->stream_idr, vvd->gvm_stream_id);
		req_p = virtio_video_alloc_req_resp(vvd,
		                                    NULL,
		                                    &vbuf,
		                                    sizeof(*req_p),
		                                    0,
		                                    NULL);
		req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_CLOSE_GVM);
		req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
		ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
		if (ret == -ETIMEDOUT) {
		    vpr_e(vvd2tag(vvd), "timed out waiting for close gvm\n");
		}
		virtio_video_cmd_stream_destroy(vvd, vvd->gvm_stream_id);
		virtio_video_stream_id_put(vvd, vvd->gvm_stream_id);
		kfree(stream);
		vvd->gvm_stream_id = 0;
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_cmd_close_gvm);

static void
virtio_video_cmd_open_gvm_session_cb(struct virtio_video_device *vvd,
				     struct virtio_video_vbuffer *vbuf)
{
	struct virtio_video_open_gvm_session_resp *resp =
		(struct virtio_video_open_gvm_session_resp *)vbuf->resp_buf;

	vvd->device_id = le32_to_cpu(resp->device_id);
	vvd->session_id = le32_to_cpu(resp->session_id);
	vvd->session_handle = le64_to_cpu(resp->session_handle);
}

int32_t virtio_video_cmd_open_gvm_session(uint32_t vm_id, uint32_t* device_id, uint32_t* session_id)
{
	int32_t ret = 0;
	struct virtio_video_open_gvm_session *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virtualization_get_vvd();
	size_t resp_size = sizeof(struct virtio_video_open_gvm_session_resp);

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		ret = -EXDEV;
	} else {
		if (vvd->session_handle == 0) {
			req_p = virtio_video_alloc_req_resp(vvd,
			                                    virtio_video_cmd_open_gvm_session_cb,
			                                    &vbuf,
			                                    sizeof(*req_p),
			                                    resp_size,
			                                    NULL);
			req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_OPEN_GVM_SESSION);
			req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
			req_p->vm_id = cpu_to_le32(vm_id);
			ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
			if (ret == -ETIMEDOUT) {
				vpr_e(vvd2tag(vvd), "timed out waiting for open gvm session\n");
			} else if (vvd->device_id == 0 || vvd->session_id == 0
					 || vvd->session_handle == 0) {
				vpr_e(vvd2tag(vvd), "open GVM session BE returns NULL\n");
				ret = -EINVAL;
			} else {
				*device_id = vvd->device_id;
				*session_id = vvd->session_id;
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_cmd_open_gvm_session);

int32_t virtio_video_cmd_pause_gvm_session(uint32_t device_id, uint32_t session_id)
{
	int32_t ret = 0;
	struct virtio_video_gvm_session *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virtualization_get_vvd();

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		ret = -EXDEV;
	} else {
		if (vvd->session_handle != 0) {
			req_p = virtio_video_alloc_req_resp(vvd,
			                                    NULL,
			                                    &vbuf,
			                                    sizeof(*req_p),
			                                    0,
			                                    NULL);
			req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_PAUSE_GVM_SESSION);
			req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
			req_p->device_id = cpu_to_le32(device_id);
			req_p->session_id = cpu_to_le32(session_id);
			req_p->session_handle = cpu_to_le64(vvd->session_handle);
			ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
			if (ret == -ETIMEDOUT) {
			    vpr_e(vvd2tag(vvd), "timed out waiting for pause gvm session\n");
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_cmd_pause_gvm_session);

int32_t virtio_video_cmd_resume_gvm_session(uint32_t device_id, uint32_t session_id)
{
	int32_t ret = 0;
	struct virtio_video_gvm_session *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virtualization_get_vvd();

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		ret = -EXDEV;
	} else {
		if (vvd->session_handle != 0) {
			req_p = virtio_video_alloc_req_resp(vvd,
			                                    NULL,
			                                    &vbuf,
			                                    sizeof(*req_p),
			                                    0,
			                                    NULL);
			req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_RESUME_GVM_SESSION);
			req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
			req_p->device_id = cpu_to_le32(device_id);
			req_p->session_id = cpu_to_le32(session_id);
			req_p->session_handle = cpu_to_le64(vvd->session_handle);
			ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
			if (ret == -ETIMEDOUT) {
			    vpr_e(vvd2tag(vvd), "timed out waiting for resume gvm session\n");
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_cmd_resume_gvm_session);

#endif