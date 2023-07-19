// SPDX-License-Identifier: GPL-2.0+
/* Driver for virtio video device.
 *
 * Copyright 2020 OpenSynergy GmbH.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/module.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)
#include <linux/dma-map-ops.h>
#else
#include <linux/dma-mapping.h>
#endif

#include "virtio_video.h"

#ifdef CONFIG_MSM_VIRTIO_HAB
#include <linux/habmm.h>
extern struct virtio_device * virthab_get_vdev(int32_t mmid);
#endif

#define NUM_VIDEO_DEVICE 3

static unsigned int debug;
module_param(debug, uint, 0644);

static unsigned int use_dma_mem;
module_param(use_dma_mem, uint, 0644);
MODULE_PARM_DESC(use_dma_mem, "Try to allocate buffers from the DMA zone");

static atomic_t v4l2_instance = ATOMIC_INIT(0);

static int vid_nr_dec = 32;
module_param(vid_nr_dec, int, 0644);
MODULE_PARM_DESC(vid_nr_dec, "videoN start number, -1 is autodetect");

static int vid_nr_enc = 33;
module_param(vid_nr_enc, int, 0644);
MODULE_PARM_DESC(vid_nr_enc, "videoN start number, -1 is autodetect");

static int vid_nr_cam = -1;
module_param(vid_nr_cam, int, 0644);
MODULE_PARM_DESC(vid_nr_cam, "videoN start number, -1 is autodetect");

static bool mplane_cam = true;
module_param(mplane_cam, bool, 0644);
MODULE_PARM_DESC(mplane_cam,
	"1 (default) - multiplanar camera, 0 - single planar camera");

static int virtio_video_probe(struct virtio_device* vdev)
{
	int ret = 0;
	struct virtio_video_device *vvd;
#ifndef CONFIG_MSM_VIRTIO_HAB
	struct virtqueue *vqs[2];
#endif
	struct device *dev = &vdev->dev;
#ifndef CONFIG_MSM_VIRTIO_HAB
	struct device *pdev = dev->parent;
#endif
#ifndef CONFIG_MSM_VIRTIO_HAB
	static const char * const names[] = { "commandq", "eventq" };
	static vq_callback_t *callbacks[] = {
		virtio_video_cmd_cb,
		virtio_video_event_cb
	};
#endif

	if (!virtio_has_feature(vdev, VIRTIO_VIDEO_F_RESOURCE_GUEST_PAGES)) {
		dev_err(dev, "device must support guest allocated buffers\n");
		return -ENODEV;
	}

	vvd = devm_kzalloc(dev, sizeof(*vvd), GFP_KERNEL);
	if (!vvd)
		return -ENOMEM;

	vvd->is_m2m_dev = true;

	switch (vdev->id.device) {
#ifdef VIRTIO_VIDEO_CAM_SUPPORT
	case VIRTIO_ID_VIDEO_CAM:
		vvd->is_m2m_dev = false;
		vvd->vid_dev_nr = vid_nr_cam;
		vvd->is_mplane_cam = mplane_cam;
		vvd->type = VIRTIO_VIDEO_DEVICE_CAMERA;
		break;
#endif
	case VIRTIO_ID_VIDEO_ENCODER:
		vvd->vid_dev_nr = vid_nr_enc;
		vvd->type = VIRTIO_VIDEO_DEVICE_ENCODER;
		break;
	case VIRTIO_ID_VIDEO_DECODER:
	default:
		vvd->vid_dev_nr = vid_nr_dec;
		vvd->type = VIRTIO_VIDEO_DEVICE_DECODER;
		break;
	}

	vvd->vdev = vdev;
	vvd->debug = debug;
	vvd->use_dma_mem = use_dma_mem;

	vdev->priv = vvd;

	spin_lock_init(&vvd->pending_buf_list_lock);
	spin_lock_init(&vvd->resource_idr_lock);
	idr_init(&vvd->resource_idr);
	spin_lock_init(&vvd->stream_idr_lock);
	idr_init(&vvd->stream_idr);

	init_waitqueue_head(&vvd->wq);

	if (virtio_has_feature(vdev, VIRTIO_VIDEO_F_RESOURCE_NON_CONTIG))
		vvd->supp_non_contig = true;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
	vvd->has_iommu = !virtio_has_dma_quirk(vdev);
#else
	vvd->has_iommu = !virtio_has_iommu_quirk(vdev);
#endif
#ifndef CONFIG_MSM_VIRTIO_HAB
	if (!dev->dma_ops)
		set_dma_ops(dev, pdev->dma_ops);

		/*
		* Set it to coherent_dma_mask by default if the architecture
		* code has not set it.
		*/
	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;

	dma_set_mask(dev, *pdev->dma_mask);

#endif

	v4l2_device_set_name(&vvd->v4l2_dev, DRIVER_NAME, &v4l2_instance);
#ifndef CONFIG_MSM_VIRTIO_HAB
	/* when using HAB, the dev name has been set in register_virtio_device */
	dev_set_name(dev, "%s.%i", DRIVER_NAME, vdev->index);
#endif
	ret = v4l2_device_register(dev, &vvd->v4l2_dev);
	if (ret)
		goto err_v4l2_reg;

	spin_lock_init(&vvd->commandq.qlock);
	init_waitqueue_head(&vvd->commandq.reclaim_queue);

	INIT_WORK(&vvd->eventq.work, virtio_video_process_events);

	INIT_LIST_HEAD(&vvd->pending_vbuf_list);

#ifdef CONFIG_MSM_VIRTIO_HAB
	vvd->commandq.vq = kmalloc(sizeof(struct virtqueue), GFP_KERNEL);
	vvd->commandq.vq->habmm_handle = 0;
	vvd->commandq.vq->vdev = vdev;
	vvd->commandq.vq->priv = vvd;
	spin_lock_init(&vvd->commandq.vq->qlock);
	INIT_LIST_HEAD(&vvd->commandq.vq->resp_list);
	vvd->eventq.vq = kmalloc(sizeof(struct virtqueue), GFP_KERNEL);
	vvd->eventq.vq->habmm_handle = 0;
	vvd->eventq.vq->vdev = vdev;
	vvd->eventq.vq->priv = vvd;
	spin_lock_init(&vvd->eventq.vq->qlock);
	INIT_LIST_HEAD(&vvd->eventq.vq->resp_list);
#else
	ret = virtio_find_vqs(vdev, 2, vqs, callbacks, names, NULL);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "failed to find virt queues\n");
		goto err_vqs;
	}

	vvd->commandq.vq = vqs[0];
	vvd->eventq.vq = vqs[1];
#endif
	ret = virtio_video_alloc_vbufs(vvd);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "failed to alloc vbufs\n");
		goto err_vbufs;
	}
#ifndef CONFIG_MSM_VIRTIO_HAB
	virtio_cread(vdev, struct virtio_video_config, max_caps_length,
		     &vvd->max_caps_len);
	if (!vvd->max_caps_len) {
		v4l2_err(&vvd->v4l2_dev, "max_caps_len is zero\n");
		ret = -EINVAL;
		goto err_config;
	}

	virtio_cread(vdev, struct virtio_video_config, max_resp_length,
		     &vvd->max_resp_len);
	if (!vvd->max_resp_len) {
		v4l2_err(&vvd->v4l2_dev, "max_resp_len is zero\n");
		ret = -EINVAL;
		goto err_config;
	}
#else
	/* Set non-zero value only for addressing compilation error */
	vvd->max_caps_len = MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE;
	vvd->max_resp_len = MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE;
#endif

#ifndef CONFIG_MSM_VIRTIO_HAB
	ret = virtio_video_alloc_events(vvd);
	if (ret)
		goto err_events;
	if (!once) {
		virtio_device_ready(vdev);
	}
#endif
	vvd->commandq.ready = true;
	vvd->eventq.ready = true;

	ret = virtio_video_device_init(vvd);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev,"failed to init virtio video\n");
		goto err_init;
	}

	return 0;

err_init:
#ifndef CONFIG_MSM_VIRTIO_HAB
err_events:
err_config:
#endif
	virtio_video_free_vbufs(vvd);
err_vbufs:
	vdev->config->del_vqs(vdev);
#ifndef CONFIG_MSM_VIRTIO_HAB
err_vqs:
#endif
	v4l2_device_unregister(&vvd->v4l2_dev);
err_v4l2_reg:
	devm_kfree(dev, vvd);

	return ret;
}

static void virtio_video_remove(struct virtio_device *vdev)
{
	struct virtio_video_device *vvd = vdev->priv;

	pr_info("%s %s\n", __func__, dev_name(&vdev->dev));

	virtio_video_device_deinit(vvd);
	virtio_video_free_vbufs(vvd);
#ifndef CONFIG_MSM_VIRTIO_HAB
	vdev->config->del_vqs(vdev);
#endif
	v4l2_device_unregister(&vvd->v4l2_dev);
	devm_kfree(&vdev->dev, vvd);
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_VIDEO_DECODER, VIRTIO_DEV_ANY_ID },
	{ VIRTIO_ID_VIDEO_ENCODER, VIRTIO_DEV_ANY_ID },
#ifdef VIRTIO_VIDEO_CAM_SUPPORT
	{ VIRTIO_ID_VIDEO_CAM, VIRTIO_DEV_ANY_ID },
#endif
	{ 0 },
};

static unsigned int features[] = {
	VIRTIO_VIDEO_F_RESOURCE_GUEST_PAGES,
	VIRTIO_VIDEO_F_RESOURCE_NON_CONTIG,
};

static struct virtio_driver virtio_video_driver = {
	.feature_table = features,
	.feature_table_size = ARRAY_SIZE(features),
	.driver.name = DRIVER_NAME,
	.driver.owner = THIS_MODULE,
	.id_table = id_table,
	.probe = virtio_video_probe,
	.remove = virtio_video_remove,
};

#ifndef CONFIG_MSM_VIRTIO_HAB
module_virtio_driver(virtio_video_driver);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("virtio video driver");
MODULE_AUTHOR("Dmitry Sepp <dmitry.sepp@opensynergy.com>");
MODULE_AUTHOR("Kiran Pawar <kiran.pawar@opensynergy.com>");
MODULE_AUTHOR("Nikolay Martyanov <nikolay.martyanov@opensynergy.com>");
MODULE_AUTHOR("Samiullah Khawaja <samiullah.khawaja@opensynergy.com>");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

#else

static void msm_vdev_release(struct device *dev)
{
	pr_info("%s\n", __func__);
}

static void msm_vdev_reset(struct virtio_device *dev)
{
	pr_info("%s: virtio_device is being reset!\n", __func__);
}

static void msm_vdev_set_status(struct virtio_device *dev, uint8_t status)
{
	pr_info("%s: setting status %d\n", __func__, status);
}

static uint8_t msm_vdev_get_status(struct virtio_device *dev)
{
	pr_info("%s: getting status\n", __func__);

	return 0;
}
static u64 msm_vdev_get_features(struct virtio_device *vdev)
{
	pr_info("%s: \n", __func__);

	return VIRTIO_VIDEO_F_RESOURCE_GUEST_PAGES|
	       VIRTIO_VIDEO_F_RESOURCE_NON_CONTIG;
}

static int msm_vdev_finalize_features(struct virtio_device *vdev)
{
	pr_info("%s: \n", __func__);

	return 0;
}

void msm_vdev_del_vqs(struct virtio_device *vdev)
{
	pr_info("%s: \n", __func__);
}

static const struct virtio_config_ops msm_vdev_config_ops = {
	.reset              = msm_vdev_reset,
	.set_status         = msm_vdev_set_status,
	.get_status         = msm_vdev_get_status,
	.del_vqs            = msm_vdev_del_vqs,
	.get_features       = msm_vdev_get_features,
	.finalize_features  = msm_vdev_finalize_features,
};

static struct virtio_device* venc = NULL;
static struct virtio_device* vdec = NULL;

static int __init msm_virtio_video_init(void)
{
	int ret = 0;
	struct virtio_device* vdev = NULL;

	vdev = virthab_get_vdev(MM_VID);
	if (!vdev) {
		pr_err("failed to get vdev for video\n");
		ret = -ENODEV;
		goto err;
	}

	ret = register_virtio_driver(&virtio_video_driver);
	if (ret) {
		pr_err("%s: virtio video driver registration failed\n", __func__);
		goto err;
	}

	vdec = kzalloc(sizeof(*vdec), GFP_KERNEL);
	if (vdec != NULL) {
		vdec->config = &msm_vdev_config_ops;
		vdec->id.device = VIRTIO_ID_VIDEO_DECODER;
		vdec->id.vendor = VIRTIO_DEV_ANY_ID;
		vdec->dev.parent = &vdev->dev;
		vdec->dev.release = msm_vdev_release;
		pr_info("%s: registering virtio device for video decoder\n", __func__);
		ret = register_virtio_device(vdec);
		if (ret) {
			put_device(&vdec->dev);
			pr_err("%s: virtio device for decoder registration failed\n", __func__);
			goto err_dec;
		}
	} else {
		ret = -ENOMEM;
		goto err_vdec;
	}

	venc = kzalloc(sizeof(*venc), GFP_KERNEL);
	if (venc != NULL) {
		venc->config = &msm_vdev_config_ops;
		venc->id.device = VIRTIO_ID_VIDEO_ENCODER;
		venc->id.vendor = VIRTIO_DEV_ANY_ID;
		venc->dev.parent = &vdev->dev;
		venc->dev.release = msm_vdev_release;
		pr_info("%s: registering virtio device for video encoder\n", __func__);
		ret = register_virtio_device(venc);
		if (ret) {
			put_device(&venc->dev);
			pr_err("%s: virtio device for encoder registration failed\n", __func__);
			goto err_enc;
		}
	} else {
		ret = -ENOMEM;
		goto err_venc;
	}

	return 0;

err_enc:
	kfree(venc);
err_venc:
	unregister_virtio_device(vdec);
err_dec:
	kfree(vdec);
err_vdec:
	unregister_virtio_driver(&virtio_video_driver);
err:
	return ret;
}

static void __exit msm_virtio_video_exit(void)
{
	pr_info("%s\n", __func__);

	unregister_virtio_device(vdec);
	unregister_virtio_device(venc);

	unregister_virtio_driver(&virtio_video_driver);

	kfree(vdec);
	kfree(venc);
}

module_init(msm_virtio_video_init);
module_exit(msm_virtio_video_exit);

MODULE_DESCRIPTION("MSM VirtIO-video driver");
MODULE_LICENSE("GPL");
#endif
