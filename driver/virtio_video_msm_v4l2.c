/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include "virtio_video.h"
#include "virtio_video_msm_v4l2.h"
#include "virtio_video_msm_vq.h"
#include "virtio_video_msm_vb2.h"
#include "virtio_video_msm_debug.h"
#include <linux/mutex.h>
#include <media/v4l2-event.h>
#include <linux/poll.h>
#include "vidc/media/msm_media_info.h"

#pragma GCC diagnostic ignored "-Wunused-variable"

static inline bool is_valid_v4l2_buffer(struct v4l2_buffer *buf,
					struct virtio_video_stream *inst)
{
	bool ret = false;

	if ((buf->type == INPUT_MPLANE || buf->type == OUTPUT_MPLANE) &&
		buf->length > 0)
		ret = true;
	else if (buf->type == INPUT_META_PLANE || buf->type == OUTPUT_META_PLANE)
		ret = true;

	return ret;
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
		      struct v4l2_fmtdesc *fmtdesc)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_enum_fmt(vvd, stream, fmtdesc);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_try_fmt(struct file *file, void *fh, struct v4l2_format *format)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_try_fmt(vvd, stream, format);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_s_fmt(struct file* file, void* fh,
		   struct v4l2_format* format)
{
	struct virtio_video_stream* stream = file2stream(file);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_s_fmt(vvd, stream, format);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_fmt(struct file *file, void *fh,
		   struct v4l2_format *format)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_g_fmt(vvd, stream, format);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_s_selection(struct file *file, void *fh,
			 struct v4l2_selection *sel)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_s_selection(vvd, stream, sel);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_selection(struct file *file, void *fh,
			 struct v4l2_selection *sel)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_g_selection(vvd, stream, sel);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_s_parm(struct file *file, void *fh,
		    struct v4l2_streamparm *parm)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_s_parm(vvd, stream, parm);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_parm(struct file* file, void* fh,
		    struct v4l2_streamparm* parm)
{
	struct virtio_video_stream* stream = file2stream(file);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_g_parm(vvd, stream, parm);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct virtio_video_stream* stream = ctrl2stream(ctrl);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	struct v4l2_control control;
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	control.id = ctrl->id;
	ret = virtio_video_cmd_g_ctrl(vvd, stream, &control);
	if (ret)
		goto unlock;
	ctrl->val = control.value;

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
	struct v4l2_control control;
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	control.id = ctrl->id;
	control.value = ctrl->val;
	ret = virtio_video_cmd_s_ctrl(vvd, stream, &control);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_reqbufs(struct file *file, void *fh,
		     struct v4l2_requestbuffers *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int port = 0;
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	port = v4l2_type_to_driver_port(stream, buf->type, __func__);
	if (port < 0) {
		v4l2_err(&vvd->v4l2_dev, "%s: port not found for v4l2 type %d\n",
			__func__, buf->type);
		ret = -EINVAL;
		goto unlock;
	}

	ret = vb2_reqbufs(stream->bufq[port].vb2q, buf);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: vb2_querybuf(%d) failed, %d\n",
			__func__, buf->type, ret);
		goto unlock;
	}
	ret = virtio_video_cmd_reqbufs(vvd, stream, buf);

unlock:
	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_querybuf(struct file *file, void *fh,
		      struct v4l2_buffer *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	ret = virtio_video_cmd_querybuf(vvd, stream, buf);

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_qbuf(struct file *file, void *fh,
		  struct v4l2_buffer *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	struct video_device *vdev = video_devdata(file);
	int plane = 0;
	int ret = 0;
	struct vb2_queue *queue = NULL;

	if (!stream || !vvd || !buf || !is_valid_v4l2_buffer(buf, stream)) {
		v4l2_err(&vvd->v4l2_dev,"%s: invalid params %pK %pK\n", __func__, stream, buf);
		ret = -EINVAL;
		goto exit;
	}

	queue = msm_vidc_get_vb2q(stream, buf->type, __func__);
	if (!queue) {
		v4l2_err(&vvd->v4l2_dev, "%s failed to find buffer queue\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_qbuf(queue, vdev->v4l2_dev->mdev, buf);

exit:
	put_inst(stream);

	return ret;
}

int msm_v4l2_dqbuf(struct file *file, void *fh,
		   struct v4l2_buffer *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	struct vb2_queue *queue = NULL;
	int ret = 0;

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	if (!stream || !buf || !is_valid_v4l2_buffer(buf, stream)) {
		v4l2_err(&vvd->v4l2_dev,"%s: invalid params %pK %pK\n", __func__, stream, buf);
		return -EINVAL;
	}

	queue = msm_vidc_get_vb2q(stream, buf->type, __func__);
	if (!queue) {
		v4l2_err(&vvd->v4l2_dev, "%s: failed to get vb2 queue", __func__);
		ret = -EINVAL;
		goto unlock;
	}

	ret = vb2_dqbuf(queue, buf, true);
	if (ret == -EAGAIN) {
		v4l2_info(&vvd->v4l2_dev, "%s: no more buffer to dequeue", __func__);
		goto unlock;
	} else if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: failed with %d\n", __func__, ret);
		goto unlock;
	}

unlock:
	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

	return ret;
}

int msm_v4l2_streamon(struct file *file, void *fh,
		      enum v4l2_buf_type type)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int port = 0;
	int ret = 0;

	if (!stream) {
		v4l2_err(&vvd->v4l2_dev,"%s: invalid params\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	port = v4l2_type_to_driver_port(stream, type, __func__);
	if (port < 0) {
		v4l2_err(&vvd->v4l2_dev, "%s: port not found for v4l2 type %d\n",
			__func__, type);
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_streamon(stream->bufq[port].vb2q, type);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: vb2_streamon(%d) failed, %d\n",
			__func__, type, ret);
	}

exit:
	put_inst(stream);

	return ret;
}

int msm_v4l2_streamoff(struct file *file, void *fh,
		       enum v4l2_buf_type type)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;
	int port = 0;

	if (!stream) {
		v4l2_err(&vvd->v4l2_dev,"%s: invalid params\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	port = v4l2_type_to_driver_port(stream, type, __func__);
	if (port < 0) {
		v4l2_err(&vvd->v4l2_dev, "%s: port not found for v4l2 type %d\n",
			__func__, type);
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_streamoff(stream->bufq[port].vb2q, type);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: vb2_streamoff(%d) failed, %d\n",
			__func__, type, ret);
	}

exit:
	put_inst(stream);

	return ret;
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
