/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/habmm.h>

#include "virtio_video.h"

extern struct virtio_device * virthab_get_vdev(int32_t mmid);
extern int virtio_video_probe(struct virtio_device* vdev);
extern int virtio_video_remove(struct virtio_device* vdev);

static int __init msm_virtio_video_init(void)
{
	int ret = 0;
	struct virtio_device* vdev = NULL;

	vdev = virthab_get_vdev(MM_VID);
	if (!vdev) {
		pr_err("failed to get vdev for video\n");
		goto err;
	}

	vdev->id.device = VIRTIO_ID_VIDEO_DECODER;
	ret = virtio_video_probe(vdev);

	vdev->id.device = VIRTIO_ID_VIDEO_ENCODER;
	ret = virtio_video_probe(vdev);
	if (ret) {
		pr_err("%s probe failed %d\n", __func__, ret);
		goto err;
	}

	return 0;
err:
	return -1;
}

static void __exit msm_virtio_video_exit(void)
{
	struct virtio_device* vdev = NULL;

	vdev = virthab_get_vdev(MM_VID);
	if (vdev != NULL)
		virtio_video_remove(vdev);
}

//disable it if kernel config is already done to prevent loading
module_init(msm_virtio_video_init);
module_exit(msm_virtio_video_exit);

MODULE_DESCRIPTION("MSM VirtIO-video driver");
MODULE_LICENSE("GPL");
