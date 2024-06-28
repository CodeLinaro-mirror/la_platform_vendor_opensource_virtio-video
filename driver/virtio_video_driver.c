// SPDX-License-Identifier: GPL-2.0+
/* Driver for virtio video device.
 *
 * Copyright 2020 OpenSynergy GmbH.
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "virtio_video_msm_debug.h"

#if IS_ENABLED(CONFIG_MSM_HAB)
#include <linux/habmm.h>
#include "virtio_video_msm_hab.h"
#if IS_ENABLED(CONFIG_MSM_VIRTIO_HAB)
extern struct virtio_device * virthab_get_vdev(int32_t mmid);
#endif
#endif

#define NUM_VIDEO_DEVICE 3

unsigned int debug = VPR_ERR;
module_param(debug, uint, 0644);
MODULE_PARM_DESC(debug, "Set debug logmask");

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
	struct virtqueue *vqs[2];
	struct device *dev = &vdev->dev;
#ifndef VIRTIO_VIDEO_MSM
	struct device *pdev = dev->parent;
#endif
	static const char * const names[] = { "commandq", "eventq" };
	static vq_callback_t *callbacks[] = {
		virtio_video_cmd_cb,
		virtio_video_event_cb
	};

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
#ifdef MSM_VIDC_HW_VIRT
	spin_lock_init(&vvd->pending_event_list_lock);
	spin_lock_init(&vvd->wq_lock);
#endif
	init_waitqueue_head(&vvd->wq);

	if (virtio_has_feature(vdev, VIRTIO_VIDEO_F_RESOURCE_NON_CONTIG))
		vvd->supp_non_contig = true;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
	vvd->has_iommu = !virtio_has_dma_quirk(vdev);
#else
	vvd->has_iommu = !virtio_has_iommu_quirk(vdev);
#endif
#ifndef VIRTIO_VIDEO_MSM
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
#if !IS_ENABLED(CONFIG_MSM_HAB)
	/* when using HAB, the dev name has been set in register_virtio_device */
	dev_set_name(dev, "%s.%i", DRIVER_NAME, vdev->index);
#endif
	ret = v4l2_device_register(dev, &vvd->v4l2_dev);
	if (ret)
		goto err_v4l2_reg;
#ifdef VIRTIO_VIDEO_MSM
	msm_update_device_tag(vvd);
#endif
	spin_lock_init(&vvd->commandq.qlock);
	init_waitqueue_head(&vvd->commandq.reclaim_queue);

	INIT_WORK(&vvd->eventq.work, virtio_video_process_events);

	INIT_LIST_HEAD(&vvd->pending_vbuf_list);
#ifdef MSM_VIDC_HW_VIRT
	INIT_LIST_HEAD(&vvd->pending_event_list);
#endif

	ret = virtio_find_vqs(vdev, 2, vqs, callbacks, names, NULL);
	if (ret) {
		vpr_e(vvd2tag(vvd), "failed to find virt queues\n");
		goto err_vqs;
	}

	vvd->commandq.vq = vqs[0];
	vvd->eventq.vq = vqs[1];

	ret = virtio_video_alloc_vbufs(vvd);
	if (ret) {
		vpr_e(vvd2tag(vvd), "failed to alloc vbufs\n");
		goto err_vbufs;
	}

	virtio_cread(vdev, struct virtio_video_config, max_caps_length,
		     &vvd->max_caps_len);
	if (!vvd->max_caps_len) {
		vpr_e(vvd2tag(vvd), "max_caps_len is zero\n");
		ret = -EINVAL;
		goto err_config;
	}

	virtio_cread(vdev, struct virtio_video_config, max_resp_length,
		     &vvd->max_resp_len);
	if (!vvd->max_resp_len) {
		vpr_e(vvd2tag(vvd), "max_resp_len is zero\n");
		ret = -EINVAL;
		goto err_config;
	}

#ifdef VIRTIO_VIDEO_MSM
	virtio_cread(vdev, struct virtio_video_config, version, &vvd->version);
#endif

	ret = virtio_video_alloc_events(vvd);
	if (ret)
		goto err_events;

	virtio_device_ready(vdev);

	vvd->commandq.ready = true;
	vvd->eventq.ready = true;

	ret = virtio_video_device_init(vvd);
	if (ret) {
		vpr_e(vvd2tag(vvd), "failed to init virtio video\n");
		goto err_init;
	}

	return 0;

err_init:
err_events:
err_config:
	virtio_video_free_vbufs(vvd);
err_vbufs:
	vdev->config->del_vqs(vdev);
err_vqs:
	v4l2_device_unregister(&vvd->v4l2_dev);
err_v4l2_reg:
	devm_kfree(dev, vvd);

	return ret;
}

static void virtio_video_remove(struct virtio_device *vdev)
{
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: %s\n", dev_name(&vdev->dev), __func__);

	virtio_video_device_deinit(vvd);
	virtio_video_free_vbufs(vvd);
	vdev->config->del_vqs(vdev);
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

#ifndef VIRTIO_VIDEO_MSM
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

static void msm_vdev_get(struct virtio_device *vdev, unsigned offset,
			 void *buf, unsigned len)
{
	struct virtio_video_config cfg = {0};
	const char *dev_n = dev_name(&vdev->dev);
	struct virtio_video_device *vvd = vdev->priv;

	cfg = msm_hab_get_config(vdev);
	if (offset == offsetof(struct virtio_video_config, max_caps_length)) {
		*(uint32_t *)buf = cfg.max_caps_length;
		vpr_l(vvd2tag(vvd), "%s: %s offset=%x max_caps_length=%d\n",
		      dev_n, __func__, offset, *(uint32_t *)buf);
	}
	else if (offset == offsetof(struct virtio_video_config, max_resp_length)) {
		*(uint32_t *)buf = cfg.max_resp_length;
		vpr_l(vvd2tag(vvd), "%s: %s offset=%x max_resp_length=%d\n",
		      dev_n, __func__, offset, *(uint32_t *)buf);
	}
	else if (offset == offsetof(struct virtio_video_config, version)) {
		*(uint32_t *)buf = cfg.version;
		vpr_l(vvd2tag(vvd), "%s: %s offset=%x version=%d \n",
		      dev_n, __func__, offset, *(uint32_t *)buf);
	}
	else
		vpr_e(vvd2tag(vvd), "%s: %s unsupported\n", dev_n, __func__);
}

static void msm_vdev_release(struct device *dev)
{
	struct virtio_device *vdev = container_of(dev, struct virtio_device, dev);
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: %s\n", dev_name(dev), __func__);
}

static void msm_vdev_reset(struct virtio_device *vdev)
{
	struct virtio_video_device *vvd = vdev->priv;

	if (msm_hab_vdev_init(vdev))
		vpr_e(vvd2tag(vvd), "%s: failed for video%d", __func__, vdev->id.device);
	else
		vpr_h(vvd2tag(vvd), "%s: %s\n", dev_name(&vdev->dev), __func__);
}

static void msm_vdev_set_status(struct virtio_device *vdev, uint8_t status)
{
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: %s: setting status %x\n", dev_name(&vdev->dev), __func__,
	      status);

	if (status & VIRTIO_CONFIG_S_DRIVER_OK)
		msm_hab_start(vdev);
}

static uint8_t msm_vdev_get_status(struct virtio_device *vdev)
{
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: %s: getting status\n", dev_name(&vdev->dev), __func__);

	return 0;
}

static uint64_t msm_vdev_get_features(struct virtio_device *vdev)
{
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: %s\n", dev_name(&vdev->dev), __func__);

	return msm_hab_get_features(vdev);
}

static int msm_vdev_finalize_features(struct virtio_device *vdev)
{
	int ret = 0;
	struct virtio_video_device *vvd = vdev->priv;

	vpr_h(vvd2tag(vvd), "%s: %s\n", dev_name(&vdev->dev), __func__);

	ret = msm_hab_set_features(vdev);
	if (ret)
		vpr_e(vvd2tag(vvd), "%s: failed for video%d, ret %d", __func__, vdev->id.device, ret);

	return ret;
}

static const struct virtio_config_ops msm_vdev_config_ops = {
	.get                = msm_vdev_get,
	.finalize_features  = msm_vdev_finalize_features,
	.get_features       = msm_vdev_get_features,
	.reset              = msm_vdev_reset,
	.set_status         = msm_vdev_set_status,
	.get_status         = msm_vdev_get_status,
	.find_vqs           = msm_hab_find_vqs,
	.del_vqs            = msm_hab_del_vqs,
};

static struct virtio_device* venc = NULL;
static struct virtio_device* vdec = NULL;

#ifdef MSM_VIDC_HW_VIRT
/* HW Virtualization uses only single HAB
 * channel for both encoder and decoder.
 */
struct virtio_video_device* msm_virtio_video_hw_virt_get_vvd(void)
{
	struct virtio_video_device *vvd = NULL;

	if (!vdec) {
		vpr_e(VPR_TAG,
		      "failed to get vdev for hardware virtualization\n");
	} else {
		vvd = (struct virtio_video_device *)vdec->priv;
	}

	return vvd;
}
#endif

static int __init msm_virtio_video_init(void)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_MSM_VIRTIO_HAB)
	struct virtio_device *vdev = NULL;

	vdev = virthab_get_vdev(MM_VID);
	if (!vdev) {
		vpr_e(VPR_TAG, "failed to get vdev for video\n");
		ret = -ENODEV;
		goto err;
	}
#endif

	ret = register_virtio_driver(&virtio_video_driver);
	if (ret) {
		vpr_e(VPR_TAG, "%s: driver registration failed\n", __func__);
		goto err;
	}

	vdec = kzalloc(sizeof(*vdec), GFP_KERNEL);
	if (vdec != NULL) {
		vdec->config = &msm_vdev_config_ops;
		vdec->id.device = VIRTIO_ID_VIDEO_DECODER;
		vdec->id.vendor = VIRTIO_DEV_ANY_ID;
#if IS_ENABLED(CONFIG_MSM_VIRTIO_HAB)
		vdec->dev.parent = &vdev->dev;
#endif
		vdec->dev.release = msm_vdev_release;
		vpr_h(VPR_TAG, "%s: registering virtio device for video decoder\n", __func__);
		ret = register_virtio_device(vdec);
		if (ret) {
			put_device(&vdec->dev);
			vpr_e(VPR_TAG, "%s: virtio device for decoder registration failed\n", __func__);
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
#if IS_ENABLED(CONFIG_MSM_VIRTIO_HAB)
		venc->dev.parent = &vdev->dev;
#endif
		venc->dev.release = msm_vdev_release;
		vpr_h(VPR_TAG, "%s: registering virtio device for video encoder\n", __func__);
		ret = register_virtio_device(venc);
		if (ret) {
			put_device(&venc->dev);
			vpr_e(VPR_TAG, "%s: virtio device for encoder registration failed\n", __func__);
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
	vpr_h(VPR_TAG, "virtio-video: %s\n", __func__);

	unregister_virtio_device(vdec);
	unregister_virtio_device(venc);

	unregister_virtio_driver(&virtio_video_driver);

	kfree(vdec);
	kfree(venc);
}

module_init(msm_virtio_video_init);
module_exit(msm_virtio_video_exit);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
MODULE_IMPORT_NS(DMA_BUF);
#endif
MODULE_DESCRIPTION("MSM VirtIO-video driver");
MODULE_LICENSE("GPL");
#endif
