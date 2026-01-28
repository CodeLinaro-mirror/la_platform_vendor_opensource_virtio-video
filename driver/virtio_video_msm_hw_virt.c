/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#include <linux/errno.h>
#include "virtio_video.h"
#include "virtio_video_msm_debug.h"
#include "virtio_video_msm_hab.h"
#include <linux/videodev2.h>
#include "include/virtio_video_hw_virt.h"
#include "include/vidc_hw_virt.h"

static int msm_create_hw_virt_stream(struct virtio_video_device *vvd)
{
	uint32_t stream_id = 0;
	struct virtio_video_stream *stream = NULL;
	char name[TASK_COMM_LEN]= {0};

	stream = kzalloc(sizeof(*stream), GFP_KERNEL);
	if (!stream)
		return -ENOMEM;

	get_task_comm(name, current);
	stream->client_id = INVALID_CLIENT_ID;
	stream->codec = INVALID_CODEC;
	stream->domain = cpu_to_le32(vvd->type);
	snprintf(stream->tag, sizeof(stream->tag), VPR_TAG);
	virtio_video_stream_id_get(vvd, stream, &stream_id);
	stream->stream_id = stream_id;
	vvd->gvm_stream_id = stream_id;
	virtio_video_state_reset(stream);

	return 0;
}

static void
virtio_video_msm_open_gvm_cb(struct virtio_video_device *vvd,
			     struct virtio_video_vbuffer *vbuf)
{
	struct virtio_video_open_gvm_resp *resp =
		(struct virtio_video_open_gvm_resp *)vbuf->resp_buf;
	vvd->device_core_mask = le32_to_cpu(resp->device_core_mask);
}

int32_t virtio_video_msm_cmd_open_gvm(uint32_t vm_id,
				      uint32_t device_id_mask,
				      uint32_t *device_core_mask)
{
	int32_t ret = 0;
	struct virtio_video_open_gvm *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	size_t resp_size = sizeof(struct virtio_video_open_gvm_resp);
	struct virtio_video_device *vvd = msm_virtio_video_hw_virt_get_vvd();

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		return -EXDEV;
	}

	ret = msm_create_hw_virt_stream(vvd);
	if (ret) {
		vpr_e(VPR_TAG, "%s: failed creating hw_virt stream", __func__);
		return ret;
	}
	req_p = virtio_video_alloc_req_resp(vvd,
	                                    virtio_video_msm_open_gvm_cb,
	                                    &vbuf,
	                                    sizeof(*req_p),
	                                    resp_size,
	                                    NULL);
	if (IS_ERR(req_p) || !vbuf)
		return -ENOMEM;

	req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_OPEN_GVM);
	req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
	req_p->vm_id = cpu_to_le32(vm_id);
	req_p->device_id_mask = cpu_to_le32(device_id_mask);
	ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
	if (ret == -ETIMEDOUT) {
		vpr_e(vvd2tag(vvd), "timed out waiting for open gvm\n");
	} else {
		*device_core_mask = vvd->device_core_mask;
		vvd->vm_id = vm_id;
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_msm_cmd_open_gvm);

int32_t virtio_video_msm_cmd_close_gvm(void)
{
	int32_t ret = 0;
	struct virtio_video_close_gvm *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virt_get_vvd();
	struct virtio_video_stream *stream = NULL;

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		return -EXDEV;
	}
	stream = idr_find(&vvd->stream_idr, vvd->gvm_stream_id);
	req_p = virtio_video_alloc_req_resp(vvd,
	                                    NULL,
	                                    &vbuf,
	                                    sizeof(*req_p),
	                                    0,
	                                    NULL);
	if (IS_ERR(req_p) || !vbuf)
		return -ENOMEM;

	req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_CLOSE_GVM);
	req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
	ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
	if (ret == -ETIMEDOUT)
		vpr_e(vvd2tag(vvd), "timed out waiting for close gvm\n");

	virtio_video_cmd_stream_destroy(vvd, vvd->gvm_stream_id);
	virtio_video_stream_id_put(vvd, vvd->gvm_stream_id);
	kfree(stream);
	vvd->gvm_stream_id = 0;

	return ret;
}
EXPORT_SYMBOL(virtio_video_msm_cmd_close_gvm);

static void
virtio_video_msm_open_gvm_session_cb(struct virtio_video_device *vvd,
				     struct virtio_video_vbuffer *vbuf)
{
	struct virtio_video_open_gvm_session_resp *resp =
		(struct virtio_video_open_gvm_session_resp *)vbuf->resp_buf;

	vvd->device_id = le32_to_cpu(resp->device_id);
	vvd->session_id = le32_to_cpu(resp->session_id);
}

int32_t virtio_video_msm_cmd_open_gvm_session(uint32_t* device_id,
					      uint32_t* session_id)
{
	int32_t ret = 0;
	struct virtio_video_open_gvm_session *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virt_get_vvd();
	size_t resp_size = sizeof(struct virtio_video_open_gvm_session_resp);

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		return -EXDEV;
	}

	req_p = virtio_video_alloc_req_resp(vvd,
	                                virtio_video_msm_open_gvm_session_cb,
	                                &vbuf,
	                                sizeof(*req_p),
	                                resp_size,
	                                NULL);
	if (IS_ERR(req_p) || !vbuf)
		return -ENOMEM;

	req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_OPEN_GVM_SESSION);
	req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
	req_p->vm_id = cpu_to_le32(vvd->vm_id);
	ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
	if (ret == -ETIMEDOUT) {
		vpr_e(vvd2tag(vvd), "timed out waiting for open gvm session\n");
	} else if (vvd->device_id == 0 || vvd->session_id == 0) {
		vpr_e(vvd2tag(vvd), "open GVM session BE returns NULL\n");
		ret = -EINVAL;
	} else {
		*device_id = vvd->device_id;
		*session_id = vvd->session_id;
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_msm_cmd_open_gvm_session);

int32_t virtio_video_msm_cmd_pause_gvm_session(uint32_t device_id,
					       uint32_t session_id)
{
	int32_t ret = 0;
	struct virtio_video_gvm_session *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virt_get_vvd();

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		return -EXDEV;
	}

	req_p = virtio_video_alloc_req_resp(vvd,
	                                    NULL,
	                                    &vbuf,
	                                    sizeof(*req_p),
	                                    0,
	                                    NULL);
	if (IS_ERR(req_p) || !vbuf)
		return -ENOMEM;

	req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_PAUSE_GVM_SESSION);
	req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
	req_p->device_id = cpu_to_le32(device_id);
	req_p->session_id = cpu_to_le32(session_id);
	ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
	if (ret == -ETIMEDOUT) {
		vpr_e(vvd2tag(vvd),
		      "timed out waiting for pause gvm session\n");
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_msm_cmd_pause_gvm_session);

int32_t virtio_video_msm_cmd_resume_gvm_session(uint32_t device_id,
						uint32_t session_id)
{
	int32_t ret = 0;
	struct virtio_video_gvm_session *req_p = NULL;
	struct virtio_video_vbuffer *vbuf = NULL;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virt_get_vvd();

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		return -EXDEV;
	}

	req_p = virtio_video_alloc_req_resp(vvd,
	                                    NULL,
	                                    &vbuf,
	                                    sizeof(*req_p),
	                                    0,
	                                    NULL);
	if (IS_ERR(req_p) || !vbuf)
		return -ENOMEM;

	req_p->hdr.type = cpu_to_le32(VIRTIO_VIDEO_CMD_RESUME_GVM_SESSION);
	req_p->hdr.stream_id = cpu_to_le32(vvd->gvm_stream_id);
	req_p->device_id = cpu_to_le32(device_id);
	req_p->session_id = cpu_to_le32(session_id);
	ret = virtio_video_queue_cmd_buffer_sync(vvd, vbuf);
	if (ret == -ETIMEDOUT) {
		vpr_e(vvd2tag(vvd),
		      "timed out waiting for resume gvm session\n");
	}

	return ret;
}
EXPORT_SYMBOL(virtio_video_msm_cmd_resume_gvm_session);

int virtio_video_pending_event_list_empty(struct virtio_video_device *vvd)
{
	int ret = 0;
#ifndef VIRTIO_VIDEO_MSM
	if (vvd->is_m2m_dev) {
		vpr_e(vvd2tag(vvd), "Unexpected call for m2m device!\n");
		return -EPERM;
	}
#endif
	spin_lock(&vvd->pending_event_list_lock);
	if (list_empty(&vvd->pending_event_list))
		ret = 1;
	spin_unlock(&vvd->pending_event_list_lock);

	return ret;
}

int virtio_video_pending_event_list_pop(struct virtio_video_device *vvd,
			       struct virtio_video_queuing_event **virtio_event)
{
	struct virtio_video_queuing_event *ret_event = NULL;
#ifndef VIRTIO_VIDEO_MSM
	if (vvd->is_m2m_dev) {
		vpr_e(vvd2tag(vvd), "Unexpected call for m2m device!\n");
		return -EPERM;
	}
#endif
	spin_lock(&vvd->pending_event_list_lock);
	if (list_empty(&vvd->pending_event_list)) {
		spin_unlock(&vvd->pending_event_list_lock);
		return -EAGAIN;
	}

	ret_event = list_first_entry(&vvd->pending_event_list,
				struct virtio_video_queuing_event, list);
	spin_unlock(&vvd->pending_event_list_lock);

	*virtio_event = ret_event;

	return 0;
}

int virtio_video_pending_event_list_add(struct virtio_video_device *vvd,
				struct virtio_video_queuing_event *virtio_event)
{
#ifndef VIRTIO_VIDEO_MSM
	if (vvd->is_m2m_dev) {
		vpr_e(vvd2tag(vvd), "Unexpected call for m2m device!\n");
		return -EPERM;
	}
#endif
	spin_lock(&vvd->pending_event_list_lock);
	list_add_tail(&virtio_event->list, &vvd->pending_event_list);
	spin_unlock(&vvd->pending_event_list_lock);

	return 0;
}

int virtio_video_pending_event_list_del(struct virtio_video_device *vvd,
				struct virtio_video_queuing_event *virtio_event)
{
	struct virtio_video_queuing_event *ve = NULL;
	struct virtio_video_queuing_event *ve_tmp = NULL;
	int ret = -EINVAL;

#ifndef VIRTIO_VIDEO_MSM
	if (vvd->is_m2m_dev) {
		vpr_e(vvd2tag(vvd), "Unexpected call for m2m device!\n");
		return -EPERM;
	}
#endif
	spin_lock(&vvd->pending_event_list_lock);
	if (list_empty(&vvd->pending_event_list)) {
		spin_unlock(&vvd->pending_event_list_lock);
		return -EAGAIN;
	}

	list_for_each_entry_safe(ve, ve_tmp, &vvd->pending_event_list, list) {
		if (ve->device_id == virtio_event->device_id) {
			list_del(&ve->list);
			ret = 0;
			break;
		}
	}
	spin_unlock(&vvd->pending_event_list_lock);

	return ret;
}

int virtio_video_queue_event_wait(struct virtio_video_msm_hw_event *evt)
{
	int ret = 0;
	struct virtio_video_device *vvd = msm_virtio_video_hw_virt_get_vvd();
	struct virtio_video_queuing_event *queuing_event;

	if (vvd == NULL) {
		vpr_e(VPR_TAG, "%s: invalid vvd", __func__);
		return -EXDEV;
	}

	if (list_empty(&vvd->pending_event_list))
		wait_event_interruptible(vvd->wq, !list_empty(&vvd->pending_event_list));

	ret = virtio_video_pending_event_list_pop(vvd, &queuing_event);
	if (ret) {
		vpr_e(vvd2tag(vvd), "unable to fetch the event\n");
	} else {
		memcpy(evt, &queuing_event->evt, sizeof(*evt));
		virtio_video_pending_event_list_del(vvd, queuing_event);
		kfree(queuing_event);
	}

        return ret;
}
EXPORT_SYMBOL(virtio_video_queue_event_wait);
