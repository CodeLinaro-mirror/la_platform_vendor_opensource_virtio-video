// SPDX-License-Identifier: GPL-2.0+
/* Decoder for virtio video device.
 *
 * Copyright 2020 OpenSynergy GmbH.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/version.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>

#include "virtio_video.h"
#include "virtio_video_msm_v4l2.h"
#include "virtio_video_msm_vb2.h"
#include "virtio_video_msm_debug.h"

static int virtio_video_dec_start_streaming(struct vb2_queue *vq,
					    unsigned int count)
{
	struct virtio_video_stream *stream = vb2_get_drv_priv(vq);

	if (virtio_video_state(stream) == STREAM_STATE_ERROR)
		return -EIO;

	if (!V4L2_TYPE_IS_OUTPUT(vq->type) &&
		virtio_video_state(stream) >= STREAM_STATE_INIT)
		virtio_video_state_update(stream, STREAM_STATE_RUNNING);

	return 0;
}

static void virtio_video_dec_stop_streaming(struct vb2_queue *vq)
{
	int queue_type;
	struct virtio_video_stream *stream = vb2_get_drv_priv(vq);

	if (V4L2_TYPE_IS_OUTPUT(vq->type))
		queue_type = VIRTIO_VIDEO_QUEUE_TYPE_INPUT;
	else
		queue_type = VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT;

	virtio_video_queue_release_buffers(stream, queue_type);
	vb2_wait_for_all_buffers(vq);
}

static const struct vb2_ops virtio_video_dec_qops = {
	.queue_setup	 = virtio_video_queue_setup,
	.buf_init	 = virtio_video_buf_init,
	.buf_cleanup	 = virtio_video_buf_cleanup,
	.buf_queue	 = virtio_video_buf_queue,
	.start_streaming = virtio_video_dec_start_streaming,
	.stop_streaming  = virtio_video_dec_stop_streaming,
	.wait_prepare	 = vb2_ops_wait_prepare,
	.wait_finish	 = vb2_ops_wait_finish,
};

static struct vb2_ops virtio_video_msm_vb2_ops = {
	.queue_setup = msm_vidc_queue_setup,
	.start_streaming = msm_vidc_start_streaming,
	.buf_queue = msm_vidc_buf_queue,
	.buf_cleanup = msm_vidc_buf_cleanup,
	.stop_streaming = msm_vidc_stop_streaming,
	.buf_out_validate = msm_vidc_buf_out_validate,
	.buf_request_complete = msm_vidc_buf_request_complete,
};

#ifndef VIRTIO_VIDEO_MSM
static int virtio_video_dec_g_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	struct virtio_video_stream *stream = ctrl2stream(ctrl);

	if (virtio_video_state(stream) == STREAM_STATE_ERROR)
		return -EIO;

	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		if (virtio_video_state(stream) >=
			STREAM_STATE_DYNAMIC_RES_CHANGE)
			ctrl->val = stream->out_info.min_buffers;
		else
			ctrl->val = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#endif

static const struct v4l2_ctrl_ops virtio_video_dec_ctrl_ops = {
#ifdef VIRTIO_VIDEO_MSM
	.g_volatile_ctrl = msm_v4l2_op_g_volatile_ctrl,
	.s_ctrl = msm_v4l2_op_s_ctrl,
#else
	.g_volatile_ctrl = virtio_video_dec_g_ctrl,
#endif
};

int virtio_video_dec_init_ctrls(struct virtio_video_stream *stream)
{
#ifdef VIRTIO_VIDEO_MSM
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	struct v4l2_ctrl_config ctrl_cfg = {0};
	int num_ctrls = 0;
	struct virtio_video_ctrl_entry *entry = NULL;
	struct virtio_video_ctrl_config *config = NULL;
	struct v4l2_ctrl *ctrl = NULL;

	list_for_each_entry(entry, &vvd->ctrl_config_list, ctrls_list_entry) {
		num_ctrls++;
	}
	v4l2_ctrl_handler_init(&stream->ctrl_handler, num_ctrls);

	list_for_each_entry(entry, &vvd->ctrl_config_list, ctrls_list_entry) {
		memset(&ctrl_cfg, 0, sizeof(ctrl_cfg));
		config = entry->config;

		vpr_h(strm2tag(stream),"%s: add ctrl, id=%#x, type=%#x, flags=%#x, max=%#llx, min=%#llx, step=%#llx, def=%#llx, name=%s, is_private=%d\n",
		      __func__, config->id, config->type, config->flags, config->max,
		      config->min, config->step, config->def,
		      (char*)config + config->name_offset, config->is_private);

		if (is_priv_ctrl(config->id)) {
			vpr_h(strm2tag(stream),"%s: add private ctrl", __func__);
			ctrl_cfg.ops = &virtio_video_dec_ctrl_ops;
			ctrl_cfg.id = config->id,
			ctrl_cfg.name = (char*)config + config->name_offset;
			ctrl_cfg.min = config->min;
			ctrl_cfg.max = config->max;
			ctrl_cfg.def = config->def;
			ctrl_cfg.flags = config->flags;
			ctrl_cfg.type = (enum v4l2_ctrl_type)config->type;

			if (ctrl_cfg.type == V4L2_CTRL_TYPE_MENU) {
				ctrl_cfg.menu_skip_mask = ~(config->step);
				ctrl_cfg.qmenu = (const char * const *)(
						 (char*)config +
						 config->qmenu_offset);
				ctrl_cfg.step = 0;
			} else {
				ctrl_cfg.step = config->step;
			}

			ctrl = v4l2_ctrl_new_custom(&stream->ctrl_handler, &ctrl_cfg, NULL);
		} else {
			vpr_h(strm2tag(stream),"%s: add std ctrl", __func__);

			if (config->type == (int)V4L2_CTRL_TYPE_MENU) {
				ctrl = v4l2_ctrl_new_std_menu(&stream->ctrl_handler,
					&virtio_video_dec_ctrl_ops, config->id, config->max,
					~(config->step), config->def);
			} else {
				ctrl = v4l2_ctrl_new_std(&stream->ctrl_handler,
					&virtio_video_dec_ctrl_ops, config->id, config->min,
					config->max, config->step, config->def);
			}

			if (ctrl && (config->flags & V4L2_CTRL_FLAG_VOLATILE))
				ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

			if (ctrl && (config->flags & V4L2_CTRL_FLAG_EXECUTE_ON_WRITE))
				ctrl->flags |= V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;
		}

		if (stream->ctrl_handler.error) {
			vpr_e(strm2tag(stream),"%s: failed to add ctrl, id=%#x, type=%#x, flags=%#x, max=%#llx, min=%#llx, step=%#llx, def=%#llx, name=%s, is_private=%d\n",
			      __func__, config->id, config->type, config->flags,
			      config->max, config->min, config->step, config->def,
			      (char*)config + config->name_offset, config->is_private);

			return stream->ctrl_handler.error;
		}
	}
#else
	struct v4l2_ctrl *ctrl;

	v4l2_ctrl_handler_init(&stream->ctrl_handler, 2);

	ctrl = v4l2_ctrl_new_std(&stream->ctrl_handler,
				&virtio_video_dec_ctrl_ops,
				V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
				MIN_BUFS_MIN, MIN_BUFS_MAX, MIN_BUFS_STEP,
				MIN_BUFS_DEF);

	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (stream->ctrl_handler.error)
		return stream->ctrl_handler.error;

	(void)v4l2_ctrl_new_std(&stream->ctrl_handler, NULL,
				V4L2_CID_MIN_BUFFERS_FOR_OUTPUT,
				MIN_BUFS_MIN, MIN_BUFS_MAX, MIN_BUFS_STEP,
				stream->in_info.min_buffers);

	if (stream->ctrl_handler.error)
		return stream->ctrl_handler.error;
#endif

	v4l2_ctrl_handler_setup(&stream->ctrl_handler);

	return 0;
}

int virtio_video_dec_init_queues(void *priv, struct vb2_queue *src_vq,
				 struct vb2_queue *dst_vq)
{
	int ret = 0;
	struct virtio_video_stream *stream = priv;
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	struct device *dev = vvd->v4l2_dev.dev;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = stream;
	src_vq->buf_struct_size = sizeof(struct virtio_video_buffer);
#ifndef VIRTIO_VIDEO_MSM
	src_vq->allow_zero_bytesused = 1;
	src_vq->ops = &virtio_video_dec_qops;
	src_vq->mem_ops = virtio_video_mem_ops(vvd);
#else
	src_vq->ops = &virtio_video_msm_vb2_ops;
	src_vq->mem_ops = vvd->vb2_mem_ops;
#endif
#if (KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE)
	src_vq->min_buffers_needed = stream->in_info.min_buffers;
#endif
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &stream->vq_mutex;
	src_vq->gfp_flags = virtio_video_gfp_flags(vvd);
	src_vq->dev = dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		goto exit;
#ifdef VIRTIO_VIDEO_MSM
	stream->bufq[INPUT_PORT].vb2q = src_vq;
#endif
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = stream;
	dst_vq->buf_struct_size = sizeof(struct virtio_video_buffer);
#ifndef VIRTIO_VIDEO_MSM
	dst_vq->allow_zero_bytesused = 1;
	dst_vq->ops = &virtio_video_dec_qops;
	dst_vq->mem_ops = virtio_video_mem_ops(vvd);
#else
	dst_vq->ops = &virtio_video_msm_vb2_ops;
	dst_vq->mem_ops = vvd->vb2_mem_ops;
#endif
#if (KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE)
	dst_vq->min_buffers_needed = stream->out_info.min_buffers;
#endif
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &stream->vq_mutex;
	dst_vq->gfp_flags = virtio_video_gfp_flags(vvd);
	dst_vq->dev = dev;

#ifdef VIRTIO_VIDEO_MSM
	ret = vb2_queue_init(dst_vq);
	if (ret)
		goto fail_out_vb2q_init;

	stream->bufq[OUTPUT_PORT].vb2q = dst_vq;
	/* allocate and init vb2_queue for meta buffers */
	ret = msm_vmem_alloc(sizeof(struct vb2_queue),
			(void **)&stream->bufq[INPUT_META_PORT].vb2q, "input meta port");
	if (ret)
		goto fail_in_meta_alloc;

	/* do input meta port queues initialization */
	ret = vb2q_init(stream, stream->bufq[INPUT_META_PORT].vb2q,
		INPUT_META_PLANE, &virtio_video_msm_vb2_ops,
		vvd->vb2_mem_ops);
	if (ret)
		goto fail_in_meta_vb2q_init;

	ret = msm_vmem_alloc(sizeof(struct vb2_queue),
			(void **)&stream->bufq[OUTPUT_META_PORT].vb2q, "output meta port");
	if (ret)
		goto fail_out_meta_alloc;

	/* do output meta port queues initialization */
	ret = vb2q_init(stream, stream->bufq[OUTPUT_META_PORT].vb2q,
		OUTPUT_META_PLANE, &virtio_video_msm_vb2_ops,
		vvd->vb2_mem_ops);
	if (ret)
		goto fail_out_meta_vb2q_init;
	goto exit;

fail_out_meta_vb2q_init:
	msm_vmem_free((void **)&stream->bufq[OUTPUT_META_PORT].vb2q);
	stream->bufq[OUTPUT_META_PORT].vb2q = NULL;
fail_out_meta_alloc:
	vb2_queue_release(stream->bufq[INPUT_META_PORT].vb2q);
fail_in_meta_vb2q_init:
	msm_vmem_free((void **)&stream->bufq[INPUT_META_PORT].vb2q);
	stream->bufq[INPUT_META_PORT].vb2q = NULL;
fail_in_meta_alloc:
	stream->bufq[OUTPUT_PORT].vb2q = NULL;
fail_out_vb2q_init:
	stream->bufq[INPUT_PORT].vb2q = NULL;

exit:
	return ret;
#else
	return vb2_queue_init(dst_vq);
#endif
}

static int virtio_video_try_decoder_cmd(struct file *file, void *fh,
					struct v4l2_decoder_cmd *cmd)
{
	struct virtio_video_stream *stream = file2stream(file);

	if (virtio_video_state(stream) == STREAM_STATE_ERROR)
		return -EIO;

	if (virtio_video_state(stream) == STREAM_STATE_DRAIN)
		return -EBUSY;

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
	case V4L2_DEC_CMD_START:
		if (cmd->flags != 0) {
			vpr_e(strm2tag(stream), "flags=%u are not supported",
			      cmd->flags);
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int virtio_video_decoder_cmd(struct file *file, void *fh,
				    struct v4l2_decoder_cmd *cmd)
{
	int ret;
	struct vb2_queue *src_vq, *dst_vq;
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = video_drvdata(file);

	ret = virtio_video_try_decoder_cmd(file, fh, cmd);
	if (ret < 0)
		return ret;

	dst_vq = v4l2_m2m_get_vq(stream->fh.m2m_ctx,
				 V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_START:
		vb2_clear_last_buffer_dequeued(dst_vq);
		break;
	case V4L2_DEC_CMD_STOP:
		src_vq = v4l2_m2m_get_vq(stream->fh.m2m_ctx,
					 V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);

		if (!vb2_is_streaming(src_vq)) {
			v4l2_dbg(1, vvd->debug,
				 &vvd->v4l2_dev, "output is not streaming\n");
			return 0;
		}

		if (!vb2_is_streaming(dst_vq)) {
			v4l2_dbg(1, vvd->debug,
				 &vvd->v4l2_dev, "capture is not streaming\n");
			return 0;
		}

		ret = virtio_video_cmd_stream_drain(vvd, stream->stream_id);
		if (ret) {
			vpr_e(strm2tag(stream), "failed to drain stream\n");
			return ret;
		}

		virtio_video_state_update(stream, STREAM_STATE_DRAIN);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int virtio_video_dec_enum_fmt_vid_cap(struct file *file, void *fh,
					     struct v4l2_fmtdesc *f)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	struct video_format_info *info;
	struct video_format *fmt;
	unsigned long input_mask = 0;
	int idx = 0, bit_num = 0;

	if (virtio_video_state(stream) == STREAM_STATE_ERROR)
		return -EIO;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	if (f->index >= vvd->num_output_fmts)
		return -EINVAL;

	info = &stream->in_info;
	list_for_each_entry(fmt, &vvd->input_fmt_list, formats_list_entry) {
		if (info->fourcc_format == fmt->desc.format) {
			input_mask = fmt->desc.mask;
			break;
		}
	}

	if (input_mask == 0)
		return -EINVAL;

	list_for_each_entry(fmt, &vvd->output_fmt_list, formats_list_entry) {
		if (test_bit(bit_num, &input_mask)) {
			if (f->index == idx) {
				f->pixelformat = fmt->desc.format;
				return 0;
			}
			idx++;
		}
		bit_num++;
	}
	return -EINVAL;
}


int virtio_video_dec_enum_fmt_vid_out(struct file *file, void *fh,
				      struct v4l2_fmtdesc *f)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	struct video_format *fmt;
	int idx = 0;

	if (virtio_video_state(stream) == STREAM_STATE_ERROR)
		return -EIO;

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	if (f->index >= vvd->num_input_fmts)
		return -EINVAL;

	list_for_each_entry(fmt, &vvd->input_fmt_list, formats_list_entry) {
		if (f->index == idx) {
			f->pixelformat = fmt->desc.format;
			return 0;
		}
		idx++;
	}
	return -EINVAL;
}

static int virtio_video_dec_s_fmt(struct file *file, void *fh,
				  struct v4l2_format *f)
{
	int ret;
	struct virtio_video_stream *stream = file2stream(file);

	ret = virtio_video_s_fmt(file, fh, f);
	if (ret)
		return ret;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		if (virtio_video_state(stream) == STREAM_STATE_IDLE)
			virtio_video_state_update(stream, STREAM_STATE_INIT);
	}

	return 0;
}

static int virtio_video_dec_s_selection(struct file *file, void *fh,
					struct v4l2_selection *sel)
{
	struct virtio_video_stream *stream = file2stream(file);
	struct virtio_video_device *vvd = to_virtio_vd(stream->video_dev);
	int ret;

	if (V4L2_TYPE_IS_OUTPUT(sel->type))
		return -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_COMPOSE:
		stream->out_info.crop.top = sel->r.top;
		stream->out_info.crop.left = sel->r.left;
		stream->out_info.crop.width = sel->r.width;
		stream->out_info.crop.height = sel->r.height;
		break;
	default:
		return -EINVAL;
	}

	ret = virtio_video_cmd_set_params(vvd, stream,  &stream->out_info,
					   VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT);
	if (ret)
		return -EINVAL;

	return virtio_video_cmd_get_params(vvd, stream,
					   VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT);
}

static const struct v4l2_ioctl_ops virtio_video_msm_dec_ioctl_ops = {
	.vidioc_querycap = msm_v4l2_querycap,
	.vidioc_enum_fmt_vid_cap = msm_v4l2_enum_fmt,
	.vidioc_enum_fmt_vid_out = msm_v4l2_enum_fmt,
	.vidioc_enum_fmt_meta_cap = msm_v4l2_enum_fmt,
	.vidioc_enum_fmt_meta_out = msm_v4l2_enum_fmt,
	.vidioc_enum_framesizes = msm_v4l2_enum_framesizes,
	.vidioc_enum_frameintervals = msm_v4l2_enum_frameintervals,
	.vidioc_try_fmt_vid_cap_mplane = msm_v4l2_try_fmt,
	.vidioc_try_fmt_vid_out_mplane = msm_v4l2_try_fmt,
	.vidioc_try_fmt_meta_cap = msm_v4l2_try_fmt,
	.vidioc_try_fmt_meta_out = msm_v4l2_try_fmt,
	.vidioc_s_fmt_vid_cap = msm_v4l2_s_fmt,
	.vidioc_s_fmt_vid_out = msm_v4l2_s_fmt,
	.vidioc_s_fmt_vid_cap_mplane = msm_v4l2_s_fmt,
	.vidioc_s_fmt_vid_out_mplane = msm_v4l2_s_fmt,
	.vidioc_s_fmt_meta_out = msm_v4l2_s_fmt,
	.vidioc_s_fmt_meta_cap = msm_v4l2_s_fmt,
	.vidioc_g_fmt_vid_cap = msm_v4l2_g_fmt,
	.vidioc_g_fmt_vid_out = msm_v4l2_g_fmt,
	.vidioc_g_fmt_vid_cap_mplane = msm_v4l2_g_fmt,
	.vidioc_g_fmt_vid_out_mplane = msm_v4l2_g_fmt,
	.vidioc_g_fmt_meta_out = msm_v4l2_g_fmt,
	.vidioc_g_fmt_meta_cap = msm_v4l2_g_fmt,
	.vidioc_g_selection = msm_v4l2_g_selection,
	.vidioc_s_selection = msm_v4l2_s_selection,
	.vidioc_s_parm = msm_v4l2_s_parm,
	.vidioc_g_parm = msm_v4l2_g_parm,
	.vidioc_reqbufs = msm_v4l2_reqbufs,
	.vidioc_querybuf = msm_v4l2_querybuf,
	.vidioc_qbuf = msm_v4l2_qbuf,
	.vidioc_dqbuf = msm_v4l2_dqbuf,
	.vidioc_streamon = msm_v4l2_streamon,
	.vidioc_streamoff = msm_v4l2_streamoff,
	.vidioc_subscribe_event = msm_v4l2_subscribe_event,
	.vidioc_unsubscribe_event = msm_v4l2_unsubscribe_event,
	.vidioc_try_decoder_cmd = msm_v4l2_try_decoder_cmd,
	.vidioc_decoder_cmd = msm_v4l2_decoder_cmd,
};

static const struct v4l2_ioctl_ops virtio_video_dec_ioctl_ops = {
	.vidioc_querycap	= virtio_video_querycap,

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0))
	.vidioc_enum_fmt_vid_cap        = virtio_video_dec_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_out        = virtio_video_dec_enum_fmt_vid_out,
#else
	.vidioc_enum_fmt_vid_cap_mplane = virtio_video_dec_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_out_mplane = virtio_video_dec_enum_fmt_vid_out,
#endif
	.vidioc_g_fmt_vid_cap_mplane	= virtio_video_g_fmt,
	.vidioc_s_fmt_vid_cap_mplane	= virtio_video_dec_s_fmt,

	.vidioc_g_fmt_vid_out_mplane	= virtio_video_g_fmt,
	.vidioc_s_fmt_vid_out_mplane	= virtio_video_dec_s_fmt,

	.vidioc_g_selection = virtio_video_g_selection,
	.vidioc_s_selection = virtio_video_dec_s_selection,

	.vidioc_try_decoder_cmd	= virtio_video_try_decoder_cmd,
	.vidioc_decoder_cmd	= virtio_video_decoder_cmd,
	.vidioc_enum_frameintervals = virtio_video_enum_framemintervals,
	.vidioc_enum_framesizes = virtio_video_enum_framesizes,

	.vidioc_reqbufs		= virtio_video_reqbufs,
	.vidioc_querybuf	= v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf		= virtio_video_qbuf,
	.vidioc_dqbuf		= virtio_video_dqbuf,
	.vidioc_prepare_buf	= v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs	= v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf		= v4l2_m2m_ioctl_expbuf,

	.vidioc_streamon	= v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff	= v4l2_m2m_ioctl_streamoff,

	.vidioc_subscribe_event = virtio_video_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

void *virtio_video_dec_get_fmt_list(struct virtio_video_device *vvd)
{
	return &vvd->input_fmt_list;
}

static struct virtio_video_device_ops virtio_video_dec_ops = {
	.init_ctrls = virtio_video_dec_init_ctrls,
	.init_queues = virtio_video_dec_init_queues,
	.get_fmt_list = virtio_video_dec_get_fmt_list,
};

int virtio_video_dec_init(struct virtio_video_device *vvd)
{
	ssize_t num;
	struct video_device *vd = &vvd->video_dev;

#ifndef VIRTIO_VIDEO_MSM
	vd->ioctl_ops = &virtio_video_dec_ioctl_ops;
#else
	vd->ioctl_ops = &virtio_video_msm_dec_ioctl_ops;
#endif
	vvd->ops = &virtio_video_dec_ops;

	num = strscpy(vd->name, "stateful-decoder", sizeof(vd->name));
	if (num < 0)
		return num;

	return 0;
}
