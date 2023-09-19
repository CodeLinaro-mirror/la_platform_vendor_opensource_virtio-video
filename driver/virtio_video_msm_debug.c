/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define CREATE_TRACE_POINTS
#include "virtio_video_msm_debug.h"
#include "virtio_video.h"

void put_inst(struct virtio_video_stream *stream)
{

	if (!stream || !stream->video_dev) {
		pr_err("%s: invalid params\n", __func__);
		return;
	}
}

const char *v4l2_type_name(uint32_t port)
{
	switch (port) {
	case INPUT_MPLANE:      return "INPUT";
	case OUTPUT_MPLANE:     return "OUTPUT";
	case INPUT_META_PLANE:  return "INPUT_META";
	case OUTPUT_META_PLANE: return "OUTPUT_META";
	}

	return "UNKNOWN";
}

void print_vb2_buffer(const char *str, struct virtio_video_stream *stream,
		struct vb2_buffer *vb2)
{
	struct virtio_video_device* vvd  = to_virtio_vd(stream->video_dev);;

	v4l2_info(&vvd->v4l2_dev,
			"%s: %s: idx %2d fd %d off %d size %d filled %d\n",
			str, v4l2_type_name(vb2->type),
			vb2->index, vb2->planes[0].m.fd,
			vb2->planes[0].data_offset, vb2->planes[0].length,
			vb2->planes[0].bytesused);

	return;
}

const char *cmd_to_string(uint32_t cmd_type)
{
	switch (cmd_type) {
	/* Command */
	case VIRTIO_VIDEO_CMD_QUERY_CAPABILITY:
		return "QUERY_CAPABILITY";
	case VIRTIO_VIDEO_CMD_STREAM_CREATE:
		return "STREAM_CREATE";
	case VIRTIO_VIDEO_CMD_STREAM_DESTROY:
		return "STREAM_DESTROY";
	case VIRTIO_VIDEO_CMD_STREAM_DRAIN:
		return "STREAM_DRAIN";
	case VIRTIO_VIDEO_CMD_RESOURCE_ATTACH:
		return "RESOURCE_ATTACH";
	case VIRTIO_VIDEO_CMD_RESOURCE_QUEUE:
		return "RESOURCE_QUEUE";
	case VIRTIO_VIDEO_CMD_QUEUE_DETACH_RESOURCES:
		return "QUEUE_DETACH_RESOURCE";
	case VIRTIO_VIDEO_CMD_QUEUE_CLEAR:
		return "QUEUE_CLEAR";
	case VIRTIO_VIDEO_CMD_GET_PARAMS:
		return "GET_PARAMS";
	case VIRTIO_VIDEO_CMD_SET_PARAMS:
		return "SET_PARAMS";
	case VIRTIO_VIDEO_CMD_QUERY_CONTROL:
		return "QUERY_CONTROL";
	case VIRTIO_VIDEO_CMD_GET_CONTROL:
		return "GET_CONTROL";
	case VIRTIO_VIDEO_CMD_SET_CONTROL:
		return "SET_CONTROL";
	case VIRTIO_VIDEO_CMD_STREAMON:
		return "STREAMON";
	case VIRTIO_VIDEO_CMD_STREAMOFF:
		return "STREAMOFF";
	case VIRTIO_VIDEO_CMD_STREAM_START:
		return "STREAM_START";

	/* Response */
	case VIRTIO_VIDEO_RESP_OK_NODATA:
		return "RESP_OK_NODATA";
	case VIRTIO_VIDEO_RESP_OK_QUERY_CAPABILITY:
		return "RESP_OK_QUERY_CAPABILITY";
	case VIRTIO_VIDEO_RESP_OK_RESOURCE_QUEUE:
		return "RESP_OK_RESOURCE_QUEUE";
	case VIRTIO_VIDEO_RESP_OK_GET_PARAMS:
		return "RESP_OK_GET_PARAMS";
	case VIRTIO_VIDEO_RESP_OK_QUERY_CONTROL:
		return "RESP_OK_QUERY_CONTROL";
	case VIRTIO_VIDEO_RESP_OK_GET_CONTROL:
		return "RESP_OK_GET_CONTROL";

	case VIRTIO_VIDEO_RESP_ERR_INVALID_OPERATION:
		return "RESP_ERR_INVALID_OPERATION";
	case VIRTIO_VIDEO_RESP_ERR_OUT_OF_MEMORY:
		return "RESP_ERR_OUT_OF_MEMORY";
	case VIRTIO_VIDEO_RESP_ERR_INVALID_STREAM_ID:
		return "RESP_ERR_INVALID_STREAM_ID";
	case VIRTIO_VIDEO_RESP_ERR_INVALID_RESOURCE_ID:
		return "RESP_ERR_INVALID_RESOURCE_ID";
	case VIRTIO_VIDEO_RESP_ERR_INVALID_PARAMETER:
		return "RESP_ERR_INVALID_PARAMETER";
	case VIRTIO_VIDEO_RESP_ERR_UNSUPPORTED_CONTROL:
		return "RESP_ERR_UNSUPPORTED_CONTROL";

	/* sub_cmd*/
	case ENUM_FMT:
		return "ENUM_FMT";
	case ENUM_FRAMESIZES:
		return "ENUM_FRAMESIZES";
	case ENUM_FRAMEINTERVALS:
		return "ENUM_FRAMEINTERVALS";
	case S_FMT:
		return "S_FMT";
	case G_FMT:
		return "G_FMT";
	case QUERYCAP:
		return "QUERYCAP";
	case SUBSCRIBE_EVENT:
		return "SUBSCRIBE_EVENT";
	case UNSUBSCRIBE_EVENT:
		return "UNSUBSCRIBE_EVENT";
	case QBUF:
		return "QBUF";
	case REQBUFS:
		return "REQBUFS";
	case G_CTRL:
		return "G_CTRL";
	case S_CTRL:
		return "S_CTRL";
	case G_PARAM:
		return "G_PARAM";
	case S_PARAM:
		return "S_PARAM";
	case G_SELECTION:
		return "G_SELECTION";
	case S_SELECTION:
		return "S_SELECTION";
	}

	return "UNKNOWN";
}

const char *codec_cmd_name(uint32_t cmd)
{
	switch (cmd) {
	case V4L2_DEC_CMD_START:
		return "START";
	case V4L2_DEC_CMD_STOP:
		return "STOP";
	case V4L2_DEC_CMD_PAUSE:
		return "PAUSE";
	case V4L2_DEC_CMD_RESUME:
		return "RESUME";
	}

	return "UNKNOWN";
}

const char *buffer_event_name(uint32_t event)
{
	switch (event) {
	case VIRTIO_VIDEO_EVENT_FBD:
		return "FBD";
	case VIRTIO_VIDEO_EVENT_EBD:
		return "EBD";
	}

	return "UNKNOWN";
}

#define DEBUG_INFO_MAX_LEN 255
char *vvd2str(struct virtio_video_device *vvd)
{
	static char debug_info[DEBUG_INFO_MAX_LEN] = "";

	if (vvd != NULL) {
		snprintf(debug_info, DEBUG_INFO_MAX_LEN, "%s : %s",
			 vvd->v4l2_dev.name, VPR_DBG_STR);
	} else {
		snprintf(debug_info, DEBUG_INFO_MAX_LEN, "%s : %s",
			 VPR_DBG_LABEL, VPR_DBG_STR);
	}

	return debug_info;
}

char *stream2str(struct virtio_video_stream *stream)
{
	static char debug_info[DEBUG_INFO_MAX_LEN] = "";

	if (stream != NULL) {
		snprintf(debug_info, DEBUG_INFO_MAX_LEN, "%s : %s",
			 stream->video_dev->v4l2_dev->name, stream->debug_str);
	} else {
		snprintf(debug_info, DEBUG_INFO_MAX_LEN, "%s : %s",
			 VPR_DBG_LABEL, VPR_DBG_STR);
	}

	return debug_info;
}
