/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _VIDC_HW_VIRT_H
#define _VIDC_HW_VIRT_H

#include <linux/types.h>
#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <vidc/linux/virtio_video.h>
#include <linux/list.h>
#include <linux/completion.h>

/*========================================================================
 Defines structure
========================================================================*/

struct virtio_video_queuing_event {
	struct virtio_video_event* evt;
	uint32_t* device_id;
	struct list_head list;
};

/*
 * virtio_video_queue_event_wait gets an event from PVM. It waits until an event is queued from PVM.
 * input: evt - an event from PVM
 * output:
 */
int virtio_video_queue_event_wait(struct virtio_video_event *evt);

int32_t virtio_video_cmd_open_gvm(uint32_t vm_id,
				  uint32_t device_id_mask,
				  uint32_t *device_core_mask);
int32_t virtio_video_cmd_close_gvm(void);
int32_t virtio_video_cmd_open_gvm_session(uint32_t* device_id, uint32_t* session_id);
int32_t virtio_video_cmd_pause_gvm_session(uint32_t device_id, uint32_t session_id);
int32_t virtio_video_cmd_resume_gvm_session(uint32_t device_id, uint32_t session_id);

#endif /* _VIDC_HW_VIRT_H */
