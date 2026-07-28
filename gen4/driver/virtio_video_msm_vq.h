/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef _VIRTIO_VIDEO_MSM_VQ_H
#define _VIRTIO_VIDEO_MSM_VQ_H
#include "virtio_video.h"

int virtio_video_cmd_enum_fmt(struct virtio_video_device* vvd,
			      struct virtio_video_stream* stream,
			      struct v4l2_fmtdesc* f);
int virtio_video_cmd_enum_frameintervals(struct virtio_video_device* vvd,
					 struct virtio_video_stream* stream,
					 struct v4l2_frmivalenum* fival);
int virtio_video_cmd_enum_framesizes(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     struct v4l2_frmsizeenum* fsize);
int virtio_video_cmd_g_fmt(struct virtio_video_device* vvd,
			   struct virtio_video_stream* stream,
			   struct v4l2_format* f);
int virtio_video_cmd_s_fmt(struct virtio_video_device* vvd,
			   struct virtio_video_stream* stream,
			   struct v4l2_format* f);
int virtio_video_cmd_g_parm(struct virtio_video_device* vvd,
			    struct virtio_video_stream* stream,
			    struct v4l2_streamparm* a);
int virtio_video_cmd_s_parm(struct virtio_video_device* vvd,
			    struct virtio_video_stream* stream,
			    struct v4l2_streamparm* a);
int virtio_video_cmd_g_ctrl(struct virtio_video_device* vvd,
			    struct virtio_video_stream* stream,
			    struct v4l2_control* c);
int virtio_video_cmd_s_ctrl(struct virtio_video_device* vvd,
			    struct virtio_video_stream* stream,
			    struct v4l2_control* c);
int virtio_video_cmd_g_selection(struct virtio_video_device* vvd,
				 struct virtio_video_stream* stream,
				 struct v4l2_selection* s);
int virtio_video_cmd_s_selection(struct virtio_video_device* vvd,
				 struct virtio_video_stream* stream,
				 struct v4l2_selection* s);
int virtio_video_cmd_querybuf(struct virtio_video_device* vvd,
			      struct virtio_video_stream* stream,
			      struct v4l2_buffer* b);
int virtio_video_cmd_querycap(struct virtio_video_device* vvd,
			      struct virtio_video_stream* stream,
			      struct v4l2_capability* cap);
int virtio_video_cmd_reqbufs(struct virtio_video_device* vvd,
			     struct virtio_video_stream* stream,
			     struct v4l2_requestbuffers* b);
int virtio_video_cmd_qbuf(struct virtio_video_device* vvd,
			  struct virtio_video_stream* stream,
			  struct v4l2_buffer* b, void* priv);
int virtio_video_cmd_subscribe_event(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     const struct v4l2_event_subscription* sub);
int virtio_video_cmd_unsubscribe_event(struct virtio_video_device* vvd,
				       struct virtio_video_stream* stream,
				       const struct v4l2_event_subscription* sub);
int virtio_video_cmd_streamoff(struct virtio_video_device* vvd,
			       struct virtio_video_stream* stream,
			       enum v4l2_buf_type i);
int virtio_video_cmd_streamon(struct virtio_video_device* vvd,
			      struct virtio_video_stream* stream,
			      enum v4l2_buf_type i);
int virtio_video_cmd_decoder_cmd(struct virtio_video_device* vvd,
				 struct virtio_video_stream* stream,
				 struct v4l2_decoder_cmd* dec);
int virtio_video_cmd_encoder_cmd(struct virtio_video_device* vvd,
				 struct virtio_video_stream* stream,
				 struct v4l2_encoder_cmd* enc);
int virtio_video_cmd_try_decoder_cmd(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     struct v4l2_decoder_cmd* dec);
int virtio_video_cmd_try_encoder_cmd(struct virtio_video_device* vvd,
				     struct virtio_video_stream* stream,
				     struct v4l2_encoder_cmd* enc);
int virtio_video_cmd_try_fmt(struct virtio_video_device* vvd,
			     struct virtio_video_stream* stream,
			     struct v4l2_format* f);

#endif //_VIRTIO_VIDEO_MSM_VQ_H
