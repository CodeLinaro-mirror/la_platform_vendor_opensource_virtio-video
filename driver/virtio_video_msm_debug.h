/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _VIRTIO_VIDEO_MSM_DEBUG_H_
#define _VIRTIO_VIDEO_MSM_DEBUG_H_

#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include "virtio_video.h"
#include "vidc/media/v4l2_vidc_extensions.h"

void print_vb2_buffer(const char *str, struct virtio_video_stream *inst,
		struct vb2_buffer *vb2);
void put_inst(struct virtio_video_stream* inst);
const char* v4l2_type_name(uint32_t port);
const char *cmd_to_string(uint32_t cmd_type);

#endif //_VIRTIO_VIDEO_MSM_DEBUG_H_
