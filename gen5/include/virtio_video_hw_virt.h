/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _VIRTIO_VIDEO_HW_VIRT_H
#define _VIRTIO_VIDEO_HW_VIRT_H

#if defined(__linux__)
#include <linux/types.h>
#else
#include "hyp_types.h"
#endif

enum virtio_video_hw_virt_cmd_type {
	/* Hardware Virtualization Command */
	VIRTIO_VIDEO_CMD_GVM_BASE = 0x00010000,
	VIRTIO_VIDEO_CMD_OPEN_GVM,
	VIRTIO_VIDEO_CMD_CLOSE_GVM,
	VIRTIO_VIDEO_CMD_OPEN_GVM_SESSION,
	VIRTIO_VIDEO_CMD_PAUSE_GVM_SESSION,
	VIRTIO_VIDEO_CMD_RESUME_GVM_SESSION,
	/* Hardware Virtualization response */
	VIRTIO_VIDEO_RESP_GVM_BASE = 0x00020000,
	VIRTIO_VIDEO_RESP_OPEN_GVM,
	VIRTIO_VIDEO_RESP_CLOSE_GVM,
	VIRTIO_VIDEO_RESP_OPEN_GVM_SESSION,
	VIRTIO_VIDEO_RESP_PAUSE_GVM_SESSION,
	VIRTIO_VIDEO_RESP_RESUME_GVM_SESSION,
};

enum virtio_video_hw_virt_event_type {
	VIRTIO_VIDEO_EVENT_GVM_SSR = 0x0300,
};

struct virtio_video_msm_msg_hdr {
	__le32 type;
	__le32 stream_id;
};

struct virtio_video_msm_msg {
	struct virtio_video_msm_msg_hdr hdr;
	__u8   payload[MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE -
		       sizeof(struct virtio_video_msm_msg_hdr)];
};

struct virtio_video_msm_cmd_msg {
	struct virtio_video_msm_msg_hdr hdr;
	__le32 sub_cmd;
	uint8_t  payload[];
};

struct virtio_video_open_gvm {
	struct virtio_video_msm_msg_hdr hdr;
	__le32 vm_id;
	__le32 device_id_mask;
};

struct virtio_video_open_gvm_resp {
	__le32 device_core_mask;
};

struct virtio_video_close_gvm {
	struct virtio_video_msm_msg_hdr hdr;
};

struct virtio_video_open_gvm_session {
	struct virtio_video_msm_msg_hdr hdr;
	__le32 vm_id;
};

struct virtio_video_open_gvm_session_resp {
	__le32 device_id;
	__le32 session_id;
	__le64 session_handle;
};

struct virtio_video_gvm_device {
	struct virtio_video_msm_msg_hdr hdr;
	__le32 device_id;
};

struct virtio_video_gvm_session {
	struct virtio_video_msm_msg_hdr hdr;
	__le32 device_id;
	__le32 session_id;
	__le64 session_handle;
};

#define VIDC_DEVICE_0               0x00000001
#define VIDC_DEVICE_1               0x00000002
#define VIDC_CORE_0                 0x00000001
#define VIDC_CORE_1                 0x00000002
#define GVM_SSR_DEVICE_DRIVER       0x80000000

#endif /* _VIRTIO_VIDEO_HW_VIRT_H */
