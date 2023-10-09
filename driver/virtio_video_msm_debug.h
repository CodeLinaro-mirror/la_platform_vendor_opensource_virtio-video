/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#if !defined(_VIRTIO_VIDEO_TRACE_) || defined(TRACE_HEADER_MULTI_READ)
#define _VIRTIO_VIDEO_TRACE_
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE virtio_video_msm_debug
#undef TRACE_SYSTEM
#define TRACE_SYSTEM virtio_video

#include <linux/tracepoint.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include "virtio_video.h"

DECLARE_EVENT_CLASS(msm_virtio_video_inst,

	TP_PROTO(char *dummy, int id),

	TP_ARGS(dummy, id),

	TP_STRUCT__entry(
		__field(char *, dummy)
		__field(int, id)
	),

	TP_fast_assign(
		__entry->dummy = dummy;
		__entry->id = id;
	),

	TP_printk("%d: %s\n", __entry->id, __entry->dummy)
);

DEFINE_EVENT(msm_virtio_video_inst, msm_virtio_video_device_open,

	TP_PROTO(char *dummy, int id),

	TP_ARGS(dummy, id)
);

DEFINE_EVENT(msm_virtio_video_inst, msm_virtio_video_device_release,

	TP_PROTO(char *dummy, int id),

	TP_ARGS(dummy, id)
);


DECLARE_EVENT_CLASS(virtio_video_buffer_queue_events,

	TP_PROTO(int id, const char *type, int index, int flags),

	TP_ARGS(id, type, index, flags),

	TP_STRUCT__entry(
		__field(int, id)
		__field(const char *, type)
		__field(int, index)
		__field(int, flags)
	),

	TP_fast_assign(
		__entry->id = id;
		__entry->type = type;
		__entry->index = index;
		__entry->flags = flags;
	),

	TP_printk(
		"%d: %s: idx %2d flags %#x\n",
		__entry->id, __entry->type, __entry->index, __entry->flags)
);

DEFINE_EVENT(virtio_video_buffer_queue_events, msm_virtio_video_qbuf,

	TP_PROTO(int id, const char *type, int index, int flags),

	TP_ARGS(id, type, index, flags)
);

DEFINE_EVENT(virtio_video_buffer_queue_events, msm_virtio_video_dqbuf,

	TP_PROTO(int id, const char *type, int index, int flags),

	TP_ARGS(id, type, index, flags)
);


DECLARE_EVENT_CLASS(virtio_video_buffer_callback_events,

	TP_PROTO(int id, const char *op, int fd, int index, int type, int flags),

	TP_ARGS(id, op, fd, index, type, flags),

	TP_STRUCT__entry(
		__field(int, id)
		__field(const char *, op)
		__field(int, fd)
		__field(int, index)
		__field(int, type)
		__field(int, flags)
	),

	TP_fast_assign(
		__entry->id = id;
		__entry->op = op;
		__entry->fd = fd;
		__entry->index = index;
		__entry->type = type;
		__entry->flags = flags;
	),

	TP_printk(
		"%d: %s: fd %#x idx %2d type %2d flags %#x\n",
		__entry->id, __entry->op, __entry->fd, __entry->index,
		__entry->type, __entry->flags)
);

DEFINE_EVENT(virtio_video_buffer_callback_events, msm_virtio_video_buffer_callback,

	TP_PROTO(int id, const char *op, int fd, int index, int type, int flags),

	TP_ARGS(id, op, fd, index, type, flags)
);

#endif //_VIRTIO_VIDEO_TRACE_

/* This part must be outside protection */
#include <trace/define_trace.h>

#ifndef _VIRTIO_VIDEO_MSM_DEBUG_H_
#define _VIRTIO_VIDEO_MSM_DEBUG_H_
void print_vb2_buffer(const char *str, struct virtio_video_stream *inst,
		struct vb2_buffer *vb2);
void put_inst(struct virtio_video_stream* inst);
const char* v4l2_type_name(uint32_t port);
const char *cmd_to_string(uint32_t cmd_type);
const char *codec_cmd_name(uint32_t cmd);
const char *buffer_event_name(uint32_t event);

#endif //_VIRTIO_VIDEO_MSM_DEBUG_H_
