/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>

#include "virtio_video.h"
#include "../../../drivers/soc/qcom/hab/hab_virtio.h"

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-function"

#ifndef CONFIG_MSM_VIRTIO_HAB
#define CONFIG_MSM_VIRTIO_HAB
#endif
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

MODULE_DESCRIPTION("MSM virtual io video driver");
MODULE_LICENSE("GPL");
