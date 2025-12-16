/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#if !defined(_VIRTIO_VIDEO_TRACE_) || defined(TRACE_HEADER_MULTI_READ)
#define _VIRTIO_VIDEO_TRACE_
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH driver
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

DECLARE_EVENT_CLASS(virtio_video_inst,
	TP_PROTO(int id),
	TP_ARGS(id),
	TP_STRUCT__entry(
		__field(int, id)
	),
	TP_fast_assign(
		__entry->id = id;
	),
	TP_printk("[%d]", __entry->id)
);

DEFINE_EVENT(virtio_video_inst, virt_vid_device_open,
	TP_PROTO(int id),
	TP_ARGS(id)
);

DEFINE_EVENT(virtio_video_inst, virt_vid_device_release,
	TP_PROTO(int id),
	TP_ARGS(id)
);

DECLARE_EVENT_CLASS(list_cnt,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt),
	TP_STRUCT__entry(
		__field(int, list_cnt)
	),
	TP_fast_assign(
		__entry->list_cnt = list_cnt;
	),
	TP_printk("cnt=%d", __entry->list_cnt)
);

DEFINE_EVENT(list_cnt, hab_resp_evt_dq_vqbuf_err,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt)
);

DEFINE_EVENT(list_cnt, hab_resp_evt_dq_vqbuf,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt)
);

DEFINE_EVENT(list_cnt, hab_resp_cmd_dq_vqbuf,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt)
);

DEFINE_EVENT(list_cnt, hab_resp_evt_done,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt)
);

DEFINE_EVENT(list_cnt, hab_resp_cmd_done,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt)
);

DEFINE_EVENT(list_cnt, hab_evt_add_vqbuf,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt)
);

DEFINE_EVENT(list_cnt, hab_cmd_add_vqbuf,
	TP_PROTO(int list_cnt),
	TP_ARGS(list_cnt)
);

DECLARE_EVENT_CLASS(virtio_video_buffer_queue_events,
	TP_PROTO(int id, int type, int index, int flags),
	TP_ARGS(id, type, index, flags),
	TP_STRUCT__entry(
		__field(int, id)
		__field(int, type)
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
		"[%d] %d: idx %2d flags %#x",
		__entry->id, __entry->type, __entry->index, __entry->flags)
);

DEFINE_EVENT(virtio_video_buffer_queue_events, virt_vid_qbuf,
	TP_PROTO(int id, int type, int index, int flags),
	TP_ARGS(id, type, index, flags)
);

DEFINE_EVENT(virtio_video_buffer_queue_events, virt_vid_dqbuf,
	TP_PROTO(int id, int type, int index, int flags),
	TP_ARGS(id, type, index, flags)
);

DECLARE_EVENT_CLASS(virtio_video_buffer_callback_events,
	TP_PROTO(int stream_id, int fd, int index, int type, int flags),
	TP_ARGS(stream_id, fd, index, type, flags),
	TP_STRUCT__entry(
		__field(int, stream_id)
		__field(int, fd)
		__field(int, index)
		__field(int, type)
		__field(int, flags)
	),
	TP_fast_assign(
		__entry->stream_id = stream_id;
		__entry->fd = fd;
		__entry->index = index;
		__entry->type = type;
		__entry->flags = flags;
	),
	TP_printk(
		"[%d] fd %#x idx %2d type %2d flags %#x",
		__entry->stream_id, __entry->fd, __entry->index,
		__entry->type, __entry->flags)
);

DEFINE_EVENT(virtio_video_buffer_callback_events, virt_vid_evt_ebd,
	TP_PROTO(int stream_id, int fd, int index, int type, int flags),
	TP_ARGS(stream_id, fd, index, type, flags)
);

DEFINE_EVENT(virtio_video_buffer_callback_events, virt_vid_evt_fbd,
	TP_PROTO(int stream_id, int fd, int index, int type, int flags),
	TP_ARGS(stream_id, fd, index, type, flags)
);

DEFINE_EVENT(virtio_video_buffer_callback_events, virt_vid_evt_rch,
	TP_PROTO(int stream_id, int fd, int index, int type, int flags),
	TP_ARGS(stream_id, fd, index, type, flags)
);

DEFINE_EVENT(virtio_video_buffer_callback_events, virt_vid_evt_err,
	TP_PROTO(int stream_id, int fd, int index, int type, int flags),
	TP_ARGS(stream_id, fd, index, type, flags)
);

#endif //_VIRTIO_VIDEO_TRACE_

/* This part must be outside protection */
#include <trace/define_trace.h>

#ifndef _VIRTIO_VIDEO_MSM_DEBUG_H_
#define _VIRTIO_VIDEO_MSM_DEBUG_H_

#define VPR_DBG_LABEL           "virtio-video"
#define VPR_DBG_STR             "core"
#define VPR_TAG                 VPR_DBG_LABEL" : "VPR_DBG_STR
static inline char *vvd2tag(struct virtio_video_device *vvd) {
	return (vvd) ? vvd->tag : (VPR_TAG);
}

static inline char *strm2tag(struct virtio_video_stream *stream) {
	return (stream) ? stream->tag : (VPR_TAG);
}

extern unsigned int debug;

enum VPR_msg_prio {
	VPR_ERR        = 0x00000001,
	VPR_HIGH       = 0x00000002,
	VPR_LOW        = 0x00000004,
};

#define e_printk(__level, __fmt, ...) \
    do { \
        if (debug & (__level)) { \
            pr_err("%s: " __fmt, \
                ##__VA_ARGS__); \
        } \
    } while (0)

#define i_printk(__level, __fmt, ...) \
    do { \
        if (debug & (__level)) { \
            pr_info("%s: " __fmt, \
                ##__VA_ARGS__); \
        } \
    } while (0)

#define vpr_e(tag, __fmt, ...) e_printk(VPR_ERR,  __fmt, tag, ##__VA_ARGS__)
#define vpr_h(tag, __fmt, ...) i_printk(VPR_HIGH, __fmt, tag, ##__VA_ARGS__)
#define vpr_l(tag, __fmt, ...) i_printk(VPR_LOW,  __fmt, tag, ##__VA_ARGS__)

void print_vb2_buffer(const char *str, struct virtio_video_stream *stream,
                      struct vb2_buffer *vb2);
void put_inst(struct virtio_video_stream* inst);
const char *v4l2_type_name(uint32_t port);
const char *cmd_to_string(uint32_t cmd_type);
const char *codec_cmd_name(uint32_t cmd);
const char *event2str(uint32_t event);
char *stream_id2tag(struct virtio_video_device *vvd, unsigned long stream_id);
void msm_update_stream_tag(struct virtio_video_stream *stream);
void msm_update_device_tag(struct virtio_video_device *vvd);

#endif //_VIRTIO_VIDEO_MSM_DEBUG_H_
