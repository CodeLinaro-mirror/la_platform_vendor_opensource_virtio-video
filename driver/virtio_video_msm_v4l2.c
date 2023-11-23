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

#define MSM_VIRTIO_VIDEO_DRV_NAME "msm_virtio_video_driver"
#define MSM_VIRTIO_VIDEO_BUS_NAME "platform:msm_virtio_video_bus"
#define MSM_VIRTIO_VIDEO_VERSION  0x1

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

static int msm_vdec_subscribe_event(struct virtio_video_stream* stream,
		const struct v4l2_event_subscription *sub)
{
	struct virtio_video_device* vvd = NULL;
	int ret = 0;

	if (!stream || !sub) {
		vpr_e(stream2str(stream),"%s: invalid params\n", __func__);
		return -EINVAL;
	}

	vvd = to_virtio_vd(stream->video_dev);

	switch (sub->type) {
	case V4L2_EVENT_EOS:
		ret = v4l2_event_subscribe(&stream->fh, sub, MAX_EVENTS, NULL);
		break;
	case V4L2_EVENT_SOURCE_CHANGE:
		ret = v4l2_src_change_event_subscribe(&stream->fh, sub);
		break;
	case V4L2_EVENT_CTRL:
		ret = v4l2_ctrl_subscribe_event(&stream->fh, sub);
		break;
	default:
		vpr_e(stream2str(stream), "%s: invalid type=%d id=%d\n", __func__,
		      sub->type, sub->id);
		ret = -EINVAL;
	}

	if (ret)
		vpr_e(stream2str(stream), "%s: failed, type=%d id=%d\n",
		      __func__, sub->type, sub->id);

	return ret;
}

static int msm_venc_subscribe_event(struct virtio_video_stream* stream,
		const struct v4l2_event_subscription *sub)
{
	struct virtio_video_device* vvd = NULL;
	int ret = 0;

	if (!stream || !sub) {
		vpr_e(stream2str(stream), "%s: invalid params\n", __func__);
		return -EINVAL;
	}

	vvd = to_virtio_vd(stream->video_dev);

	switch (sub->type) {
	case V4L2_EVENT_EOS:
		ret = v4l2_event_subscribe(&stream->fh, sub, MAX_EVENTS, NULL);
		break;
	case V4L2_EVENT_CTRL:
		ret = v4l2_ctrl_subscribe_event(&stream->fh, sub);
		break;
	case V4L2_EVENT_SOURCE_CHANGE:
		ret = v4l2_src_change_event_subscribe(&stream->fh, sub);
		break;
	default:
		vpr_e(stream2str(stream), "%s: invalid type=%d id=%d\n", __func__,
		      sub->type, sub->id);
		ret = -EINVAL;
	}

	if (ret)
		vpr_e(stream2str(stream), "%s: failed, type=%d id=%d\n",
		      __func__, sub->type, sub->id);

	return ret;
}

static int get_poll_flags(struct virtio_video_stream *stream, enum msm_vidc_port_type port)
{
	int poll = 0;
	struct vb2_queue *q = NULL;
	struct vb2_buffer *vb = NULL;
	unsigned long flags = 0;
	struct virtio_video_device *vvd = NULL;

	if (!stream || port >= MAX_PORT) {
		vpr_e(vvd2str(vvd), "%s: invalid params, inst %pK, port %d\n",
		      __func__, stream, port);
		return -EINVAL;
	}

	q = stream->bufq[port].vb2q;
	vvd = to_virtio_vd(stream->video_dev);

	spin_lock_irqsave(&q->done_lock, flags);
	if (!list_empty(&q->done_list))
		vb = list_first_entry(&q->done_list, struct vb2_buffer,
				      done_entry);

	if (vb && (vb->state == VB2_BUF_STATE_DONE ||
			vb->state == VB2_BUF_STATE_ERROR)) {
		if (port == OUTPUT_PORT || port == OUTPUT_META_PORT)
			poll |= POLLIN | POLLRDNORM;
		else if (port == INPUT_PORT || port == INPUT_META_PORT)
			poll |= POLLOUT | POLLWRNORM;
	}
	spin_unlock_irqrestore(&q->done_lock, flags);

	if (poll)
		vpr_h(stream2str(stream), "%s: got poll=%#x for port=%d\n",
		      __func__, poll, port);

	return poll;
}

unsigned int msm_v4l2_poll(struct file *file, struct poll_table_struct *pt)
{
	struct virtio_video_stream* stream = file2stream(file);
	int poll = 0;

	if (!stream || is_session_error(stream)) {
		vpr_e(stream2str(stream), "%s: invalid stream\n", __func__);
		return POLLERR;
	}

	poll_wait(file, &stream->fh.wait, pt);
	poll_wait(file, &stream->bufq[INPUT_META_PORT].vb2q->done_wq, pt);
	poll_wait(file, &stream->bufq[OUTPUT_META_PORT].vb2q->done_wq, pt);
	poll_wait(file, &stream->bufq[INPUT_PORT].vb2q->done_wq, pt);
	poll_wait(file, &stream->bufq[OUTPUT_PORT].vb2q->done_wq, pt);

	if (v4l2_event_pending(&stream->fh))
		poll |= POLLPRI;

	poll |= get_poll_flags(stream, INPUT_META_PORT);
	poll |= get_poll_flags(stream, OUTPUT_META_PORT);
	poll |= get_poll_flags(stream, INPUT_PORT);
	poll |= get_poll_flags(stream, OUTPUT_PORT);

	if (poll)
		vpr_h(stream2str(stream), "%s: return poll=%#x\n", __func__, poll);

	return poll;
}

int msm_v4l2_querycap(struct file *file, void *fh,
		      struct v4l2_capability *cap)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	strlcpy(cap->driver, MSM_VIRTIO_VIDEO_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->bus_info, MSM_VIRTIO_VIDEO_BUS_NAME, sizeof(cap->bus_info));
	cap->version = MSM_VIRTIO_VIDEO_VERSION << 16;

	ret = virtio_video_cmd_querycap(vvd, stream, cap);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: driver=%s, card=%s, bus_info=%s, version=%#x, device_cap=%#x, capabilities=%#x\n",
		      __func__, cap->driver, cap->card, cap->bus_info, cap->version,
		      cap->device_caps, cap->capabilities);

	return ret;
}

int msm_v4l2_enum_fmt(struct file *file, void *fh,
		      struct v4l2_fmtdesc *fmtdesc)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_enum_fmt(vvd, stream, fmtdesc);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: index=%#x, type=%#x, flags=%#x, pixelformat=%#x, description=%s\n",
		      __func__, fmtdesc->index, fmtdesc->type, fmtdesc->flags,
		      fmtdesc->pixelformat, fmtdesc->description);

	return ret;
}

int msm_v4l2_try_fmt(struct file *file, void *fh, struct v4l2_format *format)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_try_fmt(vvd, stream, format);

	return ret;
}

int msm_v4l2_s_fmt(struct file* file, void* fh,
		   struct v4l2_format* format)
{
	struct virtio_video_stream* stream = file2stream(file);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	if (V4L2_TYPE_IS_MULTIPLANAR(format->type)) {
		vpr_h(stream2str(stream), "%s: type=%d, width=%d, height=%d, pixelfmt=%#x, num_planes=%d, sizeimage=%d, bytesperline=%d\n",
		      __func__, format->type, format->fmt.pix_mp.width,
		      format->fmt.pix_mp.height, format->fmt.pix_mp.pixelformat,
		      format->fmt.pix_mp.num_planes,
		      format->fmt.pix_mp.plane_fmt[0].sizeimage,
		      format->fmt.pix_mp.plane_fmt[0].bytesperline);
	} else {
		vpr_h(stream2str(stream), "%s: type=%d, dataformat=%#x, buffersize=%d\n",
		      __func__, format->type, format->fmt.meta.dataformat,
		      format->fmt.meta.buffersize);
	}

	ret = virtio_video_cmd_s_fmt(vvd, stream, format);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else {
		stream->codec = format->fmt.pix_mp.pixelformat;
		msm_virtio_video_update_debug_str(stream);
	}

	return ret;
}

int msm_v4l2_g_fmt(struct file *file, void *fh,
		   struct v4l2_format *format)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_g_fmt(vvd, stream, format);

	if (ret) {
		vpr_e(stream2str(stream), "%s: failed", __func__);
	} else if (V4L2_TYPE_IS_MULTIPLANAR(format->type)) {
		vpr_h(stream2str(stream), "%s: type=%d, width=%d, height=%d, pixelfmt=%#x, num_planes=%d, sizeimage=%d, bytesperline=%d\n",
		      __func__, format->type, format->fmt.pix_mp.width,
		      format->fmt.pix_mp.height, format->fmt.pix_mp.pixelformat,
		      format->fmt.pix_mp.num_planes,
		      format->fmt.pix_mp.plane_fmt[0].sizeimage,
		      format->fmt.pix_mp.plane_fmt[0].bytesperline);
	} else {
		vpr_h(stream2str(stream), "%s: type=%d, dataformat=%#x, buffersize=%d\n",
		      __func__, format->type, format->fmt.meta.dataformat,
		      format->fmt.meta.buffersize);
	}

	return ret;
}

int msm_v4l2_s_selection(struct file *file, void *fh,
			 struct v4l2_selection *sel)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	vpr_h(stream2str(stream), "%s: type=%d, target=%d, flags=%#x\n",
	      __func__, sel->type, sel->target, sel->flags);

	ret = virtio_video_cmd_s_selection(vvd, stream, sel);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);

	return ret;
}

int msm_v4l2_g_selection(struct file *file, void *fh,
			 struct v4l2_selection *sel)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_g_selection(vvd, stream, sel);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: type=%d, target=%d, flags=%#x\n",
		      __func__, sel->type, sel->target, sel->flags);

	return ret;
}

int msm_v4l2_s_parm(struct file *file, void *fh,
		    struct v4l2_streamparm *parm)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	vpr_h(stream2str(stream), "%s: type=%d\n", __func__, parm->type);

	ret = virtio_video_cmd_s_parm(vvd, stream, parm);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);

	return ret;
}

int msm_v4l2_g_parm(struct file* file, void* fh,
		    struct v4l2_streamparm* parm)
{
	struct virtio_video_stream* stream = file2stream(file);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_g_parm(vvd, stream, parm);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: type=%d\n", __func__, parm->type);

	return ret;
}

int msm_v4l2_op_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct virtio_video_stream* stream = ctrl2stream(ctrl);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	struct v4l2_control control = {0};
	int ret = 0;

	control.id = ctrl->id;
	ret = virtio_video_cmd_g_ctrl(vvd, stream, &control);
	if (!ret)
		ctrl->val = control.value;

	if (ret)
		vpr_e(stream2str(stream), "%s: failed, ctrl_id=%#x, ctrl_value=%#x\n",
		      __func__, ctrl->id, ctrl->val);
	else
		vpr_h(stream2str(stream), "%s: ctrl_id=%#x, ctrl_value=%#x\n",
		      __func__, ctrl->id, ctrl->val);

	return ret;
}
int msm_v4l2_op_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct virtio_video_stream* stream = ctrl2stream(ctrl);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);
	struct v4l2_control control;
	int ret = 0;

	vpr_h(stream2str(stream), "%s: ctrl_id=%#x, ctrl_value=%#x\n",
	      __func__, ctrl->id, ctrl->val);

	control.id = ctrl->id;
	control.value = ctrl->val;

	if (ctrl->id == VIRTIO_VIDEO_CID_MPEG_VIDC_LAST_FLAG_EVENT_ENABLE) {
		vpr_h(stream2str(stream), "%s: enable eos event=%#x\n",
		      __func__, ctrl->val);
		stream->enable_eos_event = control.value;
	}

	if (ctrl->id == VIRTIO_VIDEO_CID_MPEG_VIDC_CLIENT_ID) {
		vpr_h(stream2str(stream), "%s: set client id=%#x\n",
		      __func__, ctrl->val);
		stream->client_id = control.value;
	}
	ret = virtio_video_cmd_s_ctrl(vvd, stream, &control);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);

	return ret;
}

int msm_v4l2_reqbufs(struct file *file, void *fh,
		     struct v4l2_requestbuffers *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int port = 0;
	int ret = 0;

	vpr_h(stream2str(stream), "%s: count=%d, type=%s, memory=%d\n",
	      __func__, buf->count, v4l2_type_name(buf->type), buf->memory);

	port = v4l2_type_to_driver_port(stream, buf->type, __func__);
	if (port < 0) {
		vpr_e(stream2str(stream), "%s: port not found for v4l2 type %s\n",
		      __func__, v4l2_type_name(buf->type));
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_reqbufs(stream->bufq[port].vb2q, buf);
	if (ret) {
		vpr_e(stream2str(stream), "%s: vb2_querybuf(%d) failed, %d\n",
		      __func__, buf->type, ret);
		goto exit;
	}
	ret = virtio_video_cmd_reqbufs(vvd, stream, buf);

exit:

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);

	return ret;
}

int msm_v4l2_querybuf(struct file *file, void *fh,
		      struct v4l2_buffer *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_querybuf(vvd, stream, buf);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: index=%d, type=%s, flags=%d\n",
		      __func__, buf->index, v4l2_type_name(buf->type), buf->flags);

	return ret;
}

int msm_v4l2_qbuf(struct file *file, void *fh,
		  struct v4l2_buffer *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	struct video_device *vdev = video_devdata(file);
	int ret = 0;
	struct vb2_queue *queue = NULL;

	if (!stream || !vvd || !buf || !is_valid_v4l2_buffer(buf, stream)) {
		vpr_e(stream2str(stream),"%s: invalid params %pK %pK\n", __func__, stream, buf);
		ret = -EINVAL;
		goto exit;
	}

	vpr_h(stream2str(stream), "%s: index=%d, type=%s, flags=%d\n",
	      __func__, buf->index, v4l2_type_name(buf->type), buf->flags);

	queue = msm_vidc_get_vb2q(stream, buf->type, __func__);
	if (!queue) {
		vpr_e(stream2str(stream), "%s failed to find buffer queue\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_qbuf(queue, vdev->v4l2_dev->mdev, buf);
	if (!ret)
		trace_virt_vid_qbuf(stream->stream_id, buf->type,
					        buf->index, buf->flags);

exit:
	put_inst(stream);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);

	return ret;
}

int msm_v4l2_dqbuf(struct file *file, void *fh,
		   struct v4l2_buffer *buf)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct vb2_queue *queue = NULL;
	int ret = 0;

	if (!stream || !buf || !is_valid_v4l2_buffer(buf, stream)) {
		vpr_e(stream2str(stream),"%s: invalid params %pK %pK\n", __func__, stream, buf);
		return -EINVAL;
	}

	vpr_h(stream2str(stream), "%s: index=%d, type=%s, flags=%d\n",
	      __func__, buf->index, v4l2_type_name(buf->type), buf->flags);

	queue = msm_vidc_get_vb2q(stream, buf->type, __func__);
	if (!queue) {
		vpr_e(stream2str(stream), "%s: failed to get vb2 queue", __func__);
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_dqbuf(queue, buf, true);
	if (ret == -EAGAIN)
		vpr_h(stream2str(stream), "%s: no more buffer to dequeue", __func__);
	else if (ret)
		vpr_e(stream2str(stream), "%s: failed with %d\n", __func__, ret);

	trace_virt_vid_dqbuf(stream->stream_id, buf->type,
				         buf->index, buf->flags);

exit:

	return ret;
}

int msm_v4l2_streamon(struct file *file, void *fh,
		      enum v4l2_buf_type type)
{
	struct virtio_video_stream *stream = file2stream(file);
	int port = 0;
	int ret = 0;

	if (!stream) {
		vpr_e(stream2str(stream),"%s: invalid params\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	port = v4l2_type_to_driver_port(stream, type, __func__);
	if (port < 0) {
		vpr_e(stream2str(stream), "%s: port not found for v4l2 type %d\n",
		      __func__, type);
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_streamon(stream->bufq[port].vb2q, type);
	if (ret) {
		vpr_e(stream2str(stream), "%s: vb2_streamon(%d) failed, %d\n",
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
	int ret = 0;
	int port = 0;

	if (!stream) {
		vpr_e(stream2str(stream),"%s: invalid params\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	port = v4l2_type_to_driver_port(stream, type, __func__);
	if (port < 0) {
		vpr_e(stream2str(stream), "%s: port not found for v4l2 type %d\n",
		      __func__, type);
		ret = -EINVAL;
		goto exit;
	}

	ret = vb2_streamoff(stream->bufq[port].vb2q, type);
	if (ret) {
		vpr_e(stream2str(stream), "%s: vb2_streamoff(%d) failed, %d\n",
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

	ret = virtio_video_cmd_subscribe_event(vvd, stream, sub);
	if (ret)
		goto exit;

	if (vvd->type == VIRTIO_VIDEO_DEVICE_DECODER)
		ret = msm_vdec_subscribe_event(stream, sub);
	if (vvd->type == VIRTIO_VIDEO_DEVICE_ENCODER)
		ret = msm_venc_subscribe_event(stream, sub);

exit:
	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: type=%d, id=%d, flags=%#x\n",
		      __func__, sub->type, sub->id, sub->flags);

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

	ret = virtio_video_cmd_unsubscribe_event(vvd, stream, sub);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: type=%d, id=%d, flags=%#x\n",
		      __func__, sub->type, sub->id, sub->flags);

	return ret;
}

int msm_v4l2_try_decoder_cmd(struct file *file, void *fh,
			     struct v4l2_decoder_cmd *dec)
{
	struct virtio_video_stream *stream = file2stream(file);
	int ret = 0;

	vpr_h(stream2str(stream), "%s: cmd %x\n", __func__, dec->cmd);
	if (dec->cmd != V4L2_DEC_CMD_STOP && dec->cmd != V4L2_DEC_CMD_START) {
		ret = -EINVAL;
		goto exit;
	}

	dec->flags = 0;
	if (dec->cmd == V4L2_DEC_CMD_STOP) {
		dec->stop.pts = 0;
	} else if (dec->cmd == V4L2_DEC_CMD_START) {
		dec->start.speed = 0;
		dec->start.format = V4L2_DEC_START_FMT_NONE;
	}

exit:

	return ret;
}

int msm_v4l2_decoder_cmd(struct file *file, void *fh,
			 struct v4l2_decoder_cmd *dec)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	vpr_h(stream2str(stream), "%s: cmd=%s, flags=%#x\n",
	      __func__, codec_cmd_name(dec->cmd), dec->flags);

	ret = virtio_video_cmd_decoder_cmd(vvd, stream, dec);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);

	return ret;
}

int msm_v4l2_try_encoder_cmd(struct file *file, void *fh,
			     struct v4l2_encoder_cmd *enc)
{
	int ret = 0;

	if (enc->cmd != V4L2_ENC_CMD_STOP && enc->cmd != V4L2_ENC_CMD_START) {
		ret = -EINVAL;
		return ret;
	}

	enc->flags = 0;

	return 0;
}

int msm_v4l2_encoder_cmd(struct file *file, void *fh,
			 struct v4l2_encoder_cmd *enc)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	vpr_h(stream2str(stream), "%s: cmd=%s, flags=%#x\n",
	      __func__, codec_cmd_name(enc->cmd), enc->flags);

	ret = virtio_video_cmd_encoder_cmd(vvd, stream, enc);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);

	return ret;
}

int msm_v4l2_enum_framesizes(struct file *file, void *fh,
			     struct v4l2_frmsizeenum *fsize)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_enum_framesizes(vvd, stream, fsize);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: index=%d, pixel_format=%#x, type=%d",
		      __func__, fsize->index, fsize->pixel_format, fsize->type);

	return ret;
}

int msm_v4l2_enum_frameintervals(struct file *file, void *fh,
				 struct v4l2_frmivalenum *fival)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret = 0;

	ret = virtio_video_cmd_enum_frameintervals(vvd, stream, fival);

	if (ret)
		vpr_e(stream2str(stream), "%s: failed", __func__);
	else
		vpr_h(stream2str(stream), "%s: index=%d, pixel_format=%#x, type=%d",
		      __func__, fival->index, fival->pixel_format, fival->type);

	return ret;
}
