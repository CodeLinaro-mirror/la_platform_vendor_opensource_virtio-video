/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/errno.h>
#include "virtio_video.h"
#include "virtio_video_msm_debug.h"
#include "virtio_video_msm_hab.h"
#include <linux/videodev2.h>

static int virtio_video_v4l2_to_hab(struct virtio_video_device* vvd,
				    uint32_t stream_id,
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
	req_p->hdr.stream_id = stream_id;
	req_p->hdr.sub_cmd_type = sub_cmd_type;

	memcpy(req_p->payload, payload, size);

	//Set vbuf.data_size to 0 for this moment
	vbuf->data_size = 0;
	vbuf->priv = priv;

	if (sync)
		ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
	else
		ret = virtio_video_queue_cmd_buffer(vvd, vbuf);

	if (ret)
		v4l2_err(&vvd->v4l2_dev, "%s: %s cmd failed. %s-%s ret %d",
		         __func__, sync ? "sync" : "async",
		         cmd_to_string(cmd_type),
		         cmd_to_string(sub_cmd_type), ret);
	else
		v4l2_info(&vvd->v4l2_dev, "%s: %s cmd done: %s-%s\n",
		          __func__, sync? "sync" : "async",
		          cmd_to_string(cmd_type),
		          cmd_to_string(sub_cmd_type));

err:
	return ret;
}

static int virtio_video_v4l2_to_hab_async(struct virtio_video_device* vvd,
					  uint32_t stream_id,
					  enum virtio_video_cmd_type cmd_type,
					  enum virtio_video_sub_cmd_type sub_cmd_type,
					  void* payload, size_t size, void* priv)
{

	return virtio_video_v4l2_to_hab(vvd, stream_id, cmd_type, sub_cmd_type,
					payload, size, priv, false);
}

static int virtio_video_v4l2_to_hab_sync(struct virtio_video_device* vvd,
					 uint32_t stream_id,
					 enum virtio_video_cmd_type cmd_type,
					 enum virtio_video_sub_cmd_type sub_cmd_type,
					 void* payload, size_t size, void* priv)
{
	return virtio_video_v4l2_to_hab(vvd, stream_id, cmd_type, sub_cmd_type,
					payload, size, priv, true);
}

int virtio_video_cmd_enum_fmt(struct virtio_video_device* vvd,
			      struct virtio_video_stream* stream,
			      struct v4l2_fmtdesc* fmtdesc)
{
	int ret = 0;

	ret = virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, ENUM_FMT,
					     (void *)fmtdesc, sizeof(*fmtdesc), NULL);

    if (!ret && !fmtdesc->pixelformat)
		ret = -EINVAL;

	return ret;
}

int virtio_video_cmd_enum_frameintervals(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_frmivalenum* fival)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, ENUM_FRAMEINTERVALS,
					     (void *)fival, sizeof(*fival), NULL);
}

int virtio_video_cmd_enum_framesizes(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_frmsizeenum* fsize)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, ENUM_FRAMESIZES,
					     (void *)fsize, sizeof(*fsize), NULL);
}

int virtio_video_cmd_g_fmt(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_format* format)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, G_FMT,
					     (void *)format, sizeof(*format), NULL);
}

int virtio_video_cmd_s_fmt(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_format* format)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, S_FMT,
					     (void *)format, sizeof(*format), NULL);
}

int virtio_video_cmd_g_parm(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_streamparm* parm)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, G_PARAM,
					     (void *)parm, sizeof(*parm), NULL);
}

int virtio_video_cmd_s_parm(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_streamparm* parm)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, S_PARAM,
					     (void *)parm, sizeof(*parm), NULL);
}

int virtio_video_cmd_g_ctrl(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_control* control)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_CONTROL, G_CTRL,
					     (void *)control, sizeof(*control), NULL);
}

int virtio_video_cmd_s_ctrl(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_control* control)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_SET_CONTROL, S_CTRL,
					     (void *)control, sizeof(*control), NULL);
}

int virtio_video_cmd_g_selection(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_selection* sel)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, G_SELECTION,
					     (void *)sel, sizeof(*sel), NULL);
}

int virtio_video_cmd_s_selection(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_selection* sel)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
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

	ret = virtio_video_v4l2_to_hab_async(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_RESOURCE_QUEUE, QBUF,
					     (void *)buf, size, priv);

	return ret;
}

int virtio_video_cmd_querybuf(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_buffer* buf)
{
	v4l2_err(&vvd->v4l2_dev, "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}

int virtio_video_cmd_querycap(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_capability* cap)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
		VIRTIO_VIDEO_CMD_GET_PARAMS, QUERYCAP,
		(void *)cap, sizeof(*cap), NULL);
}

int virtio_video_cmd_reqbufs(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, struct v4l2_requestbuffers* buf)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_GET_PARAMS, REQBUFS,
					     (void *)buf, sizeof(*buf), NULL);
}

int virtio_video_cmd_subscribe_event(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, const struct v4l2_event_subscription* sub)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, SUBSCRIBE_EVENT,
					     (void *)sub, sizeof(*sub), NULL);
}

int virtio_video_cmd_unsubscribe_event(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, const struct v4l2_event_subscription* sub)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_SET_PARAMS, UNSUBSCRIBE_EVENT,
					     (void *)sub, sizeof(*sub), NULL);
}

int virtio_video_cmd_streamon(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, enum v4l2_buf_type type)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					     VIRTIO_VIDEO_CMD_STREAMON, 0,
					     (void *)&type, sizeof(type), NULL);
}

int virtio_video_cmd_streamoff(struct virtio_video_device* vvd,
	struct virtio_video_stream* stream, enum v4l2_buf_type type)
{
	return virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
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
		v4l2_err(&vvd->v4l2_dev, "%s: invalid decoder command: %d\n", __func__, dec->cmd);
		ret = -EINVAL;
		goto err;
	}

	ret = virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id, cmd_type, 0, NULL, 0, NULL);

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
		v4l2_err(&vvd->v4l2_dev, "%s: invalid encoder command: %d\n", __func__, enc->cmd);
		ret = -EINVAL;
		goto err;
	}

	ret = virtio_video_v4l2_to_hab_sync(vvd, stream->stream_id,
					    cmd_type, 0, NULL, 0, NULL);

err:
	return ret;
}

int virtio_video_cmd_try_decoder_cmd(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     struct v4l2_decoder_cmd* dec)
{
	v4l2_err(&vvd->v4l2_dev, "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}

int virtio_video_cmd_try_encoder_cmd(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     struct v4l2_encoder_cmd* enc)
{
	v4l2_err(&vvd->v4l2_dev, "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}

int virtio_video_cmd_try_fmt(struct virtio_video_device* vvd,
			     struct virtio_video_stream* stream,
			     struct v4l2_format* format)
{
	v4l2_err(&vvd->v4l2_dev, "%s: invalid ioctl command\n", __func__);

	return -EINVAL;
}
