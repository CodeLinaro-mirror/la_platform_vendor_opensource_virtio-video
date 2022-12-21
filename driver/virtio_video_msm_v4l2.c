/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include "virtio_video.h"
#include "virtio_video_msm_v4l2.h"
#include "virtio_video_msm_vq.h"
#include "virtio_video_msm_debug.h"
#include <linux/mutex.h>
#include <media/v4l2-event.h>
#include <linux/poll.h>
#include "vidc/media/msm_media_info.h"

#pragma GCC diagnostic ignored "-Wunused-variable"

static inline bool is_decode_session(struct virtio_video_device* vvd)
{
	return vvd->type == VIRTIO_VIDEO_DEVICE_DECODER;
}

static inline bool is_encode_session(struct virtio_video_device* vvd)
{
	return vvd->type == VIRTIO_VIDEO_DEVICE_ENCODER;
}
static inline bool is_session_error(struct virtio_video_stream* stream)
{
	return virtio_video_state(stream) == STREAM_STATE_ERROR;
}

static void inst_lock(struct virtio_video_stream* inst, const char* function)
{
	mutex_lock(&inst->lock);
}

static void inst_unlock(struct virtio_video_stream* inst, const char* function)
{
	mutex_unlock(&inst->lock);
}

static void client_lock(struct virtio_video_stream* inst, const char* function)
{
	mutex_lock(&inst->client_lock);
}

static void client_unlock(struct virtio_video_stream* inst, const char* function)
{
	mutex_unlock(&inst->client_lock);
}

int msm_v4l2_querycap(struct file *file, void *fh,
			struct v4l2_capability *cap)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_querycap(vvd, stream, cap);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_enum_fmt(struct file *file, void *fh,
					struct v4l2_fmtdesc *f)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_enum_fmt(vvd, stream, f);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_try_fmt(struct file *file, void *fh, struct v4l2_format *f)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_try_fmt(vvd, stream, f);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_s_fmt(struct file* file, void* fh,
	struct v4l2_format* f)
{
	struct virtio_video_stream* stream = file2stream(file);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_s_fmt(vvd, stream, f);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_fmt(struct file *file, void *fh,
					struct v4l2_format *f)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_g_fmt(vvd, stream, f);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_s_selection(struct file *file, void *fh,
					struct v4l2_selection *s)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_s_selection(vvd, stream, s);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_selection(struct file *file, void *fh,
					struct v4l2_selection *s)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_g_selection(vvd, stream, s);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_s_parm(struct file *file, void *fh,
					struct v4l2_streamparm *a)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_s_parm(vvd, stream, a);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_parm(struct file* file, void* fh,
	struct v4l2_streamparm* a)
{
	struct virtio_video_stream* stream = file2stream(file);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_g_parm(vvd, stream, a);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct virtio_video_stream* stream = ctrl2stream(ctrl);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	struct v4l2_control c;
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	c.id = ctrl->id;
	ret = virtio_video_cmd_g_ctrl(vvd, stream, &c);
	if (ret)
		goto unlock;
	ctrl->val = c.value;

unlock:
	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}
int msm_v4l2_op_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct virtio_video_stream* stream = ctrl2stream(ctrl);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	struct v4l2_control c;
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	c.id = ctrl->id;
	c.value = ctrl->val;
	ret = virtio_video_cmd_s_ctrl(vvd, stream, &c);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_reqbufs(struct file *file, void *fh,
				struct v4l2_requestbuffers *b)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	/* TODO Implementation is in next patch*/
	int ret = 0;

	return ret;
}

int msm_v4l2_querybuf(struct file *file, void *fh,
				struct v4l2_buffer *b)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_querybuf(vvd, stream, b);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_qbuf(struct file *file, void *fh,
				struct v4l2_buffer *b)
{
	/* TODO Implementation is in next patch*/
	return 0;
}

int msm_v4l2_dqbuf(struct file *file, void *fh,
				struct v4l2_buffer *b)
{
	/* TODO Implementation is in next patch*/
	return 0;
}

int msm_v4l2_streamon(struct file *file, void *fh,
				enum v4l2_buf_type type)
{
	/* TODO Implementation is in next patch*/
	return 0;
}

int msm_v4l2_streamoff(struct file *file, void *fh,
				enum v4l2_buf_type type)
{
	/* TODO Implementation is in next patch*/
	return 0;
}

int msm_v4l2_subscribe_event(struct v4l2_fh *fh,
				const struct v4l2_event_subscription *sub)
{
	struct virtio_video_stream* stream;
	struct virtio_video_device *vvd;
	int ret = 0;

	stream = container_of(fh, struct virtio_video_stream, fh);
	vvd = to_virtio_vd(stream->video_dev);

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_subscribe_event(vvd, stream, sub);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_unsubscribe_event(struct v4l2_fh *fh,
				const struct v4l2_event_subscription *sub)
{
	struct virtio_video_stream* stream;
	struct virtio_video_device *vvd;
	int ret = 0;

	stream = container_of(fh, struct virtio_video_stream, fh);
	vvd = to_virtio_vd(stream->video_dev);
	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_unsubscribe_event(vvd, stream, sub);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_try_decoder_cmd(struct file *file, void *fh,
				 struct v4l2_decoder_cmd *dec)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	v4l2_info(&vvd->v4l2_dev, "%s: cmd %x\n", __func__, dec->cmd);
	if (dec->cmd != V4L2_DEC_CMD_STOP && dec->cmd != V4L2_DEC_CMD_START) {
		ret = -EINVAL;
		goto unlock;
	}

	dec->flags = 0;
	if (dec->cmd == V4L2_DEC_CMD_STOP) {
		dec->stop.pts = 0;
	} else if (dec->cmd == V4L2_DEC_CMD_START) {
		dec->start.speed = 0;
		dec->start.format = V4L2_DEC_START_FMT_NONE;
	}

unlock:
	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_decoder_cmd(struct file *file, void *fh,
				struct v4l2_decoder_cmd *dec)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_decoder_cmd(vvd, stream, dec);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_try_encoder_cmd(struct file *file, void *fh,
				 struct v4l2_encoder_cmd *enc)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	if (enc->cmd != V4L2_ENC_CMD_STOP && enc->cmd != V4L2_ENC_CMD_START) {
		ret = -EINVAL;
		goto unlock;
	}
	enc->flags = 0;

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

unlock:
	return ret;
}

int msm_v4l2_encoder_cmd(struct file *file, void *fh,
				struct v4l2_encoder_cmd *enc)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_encoder_cmd(vvd, stream, enc);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_enum_framesizes(struct file *file, void *fh,
				struct v4l2_frmsizeenum *fsize)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;


	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_enum_framesizes(vvd, stream, fsize);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_enum_frameintervals(struct file *file, void *fh,
				struct v4l2_frmivalenum *fival)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_enum_frameintervals(vvd, stream, fival);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_queryctrl(struct file *file, void *fh,
	struct v4l2_queryctrl *ctrl)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_queryctrl(vvd, stream, ctrl);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_querymenu(struct file *file, void *fh,
	struct v4l2_querymenu *qmenu)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_querymenu(vvd, stream, qmenu);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}
