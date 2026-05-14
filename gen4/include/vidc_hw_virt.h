/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _VIDC_HW_VIRT_H
#define _VIDC_HW_VIRT_H

#include <linux/list.h>

/*========================================================================
 Defines structure
========================================================================*/
#define MAX_HW_VIRT_CMD_PAYLOAD_SIZE (1024)

struct virtio_video_msm_hw_event {
	__le32 event_type; /* One of VIRTIO_VIDEO_EVENT_* types */
	__le32 stream_id;
	__u8 payload[MAX_HW_VIRT_CMD_PAYLOAD_SIZE - 2 * sizeof(__le32)];
};

struct virtio_video_queuing_event {
	struct virtio_video_msm_hw_event evt;
	uint32_t device_id;
	struct list_head list;
};

/*
 * virtio_video_queue_event_wait gets an event from PVM.
 * It waits until an event is queued from PVM.
 * input: evt - an event from PVM
 * output:
 */
int virtio_video_queue_event_wait(struct virtio_video_msm_hw_event *evt);

int32_t virtio_video_msm_cmd_open_gvm(uint32_t vm_id,
				      uint32_t device_id_mask,
				      uint32_t *device_core_mask);
int32_t virtio_video_msm_cmd_close_gvm(void);
int32_t virtio_video_msm_cmd_open_gvm_session(uint32_t* device_id,
					      uint32_t* session_id);
int32_t virtio_video_msm_cmd_pause_gvm_session(uint32_t device_id,
					       uint32_t session_id);
int32_t virtio_video_msm_cmd_resume_gvm_session(uint32_t device_id,
						uint32_t session_id);

#endif /* _VIDC_HW_VIRT_H */
