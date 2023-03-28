/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "virtio_video.h"
#include "virtio_video_msm_vb2.h"
#include "virtio_video_msm_vq.h"
#include "virtio_video_msm_debug.h"
#include "virtio_video_msm_mem.h"

struct vb2_queue *msm_vidc_get_vb2q(struct virtio_video_stream* stream,
				    uint32_t type, const char *func)
{
	struct vb2_queue *queue = NULL;
	struct virtio_video_device* vvd = NULL;

	if (!stream) {
		pr_err("%s: invalid stream\n", func);
		goto exit;
	}

	vvd = to_virtio_vd(stream->video_dev);

	if (type == INPUT_MPLANE) {
		queue = stream->bufq[INPUT_PORT].vb2q;
	} else if (type == OUTPUT_MPLANE) {
		queue = stream->bufq[OUTPUT_PORT].vb2q;
	} else if (type == INPUT_META_PLANE) {
		queue = stream->bufq[INPUT_META_PORT].vb2q;
	} else if (type == OUTPUT_META_PLANE) {
		queue = stream->bufq[OUTPUT_META_PORT].vb2q;
	} else {
		v4l2_err(&vvd->v4l2_dev, "%s: invalid buffer type %d\n",
			__func__, type);
	}

exit:
	return queue;
}

void *msm_vb2_attach_dmabuf(struct vb2_buffer *vb, struct device *dev,
			    struct dma_buf *dbuf, unsigned long size)
{
	return (void *)0xdeadbeef;
}

void msm_vb2_detach_dmabuf(void *buf_priv)
{
}

int msm_vb2_map_dmabuf(void *buf_priv)
{
	return 0;
}

void msm_vb2_unmap_dmabuf(void *buf_priv)
{
}

int msm_vidc_queue_setup(struct vb2_queue *queue,
			 unsigned int *num_buffers, unsigned int *num_planes,
			 unsigned int sizes[], struct device *alloc_devs[])
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device* vvd = NULL;
	int port = 0;
	int plane = 0;
	struct v4l2_format format = {0};

	if (!queue || !num_buffers || !num_planes || !sizes) {
		pr_err("%s: invalid params: queue %pK, num_buffers %pK, num_planes %pK"
			"sizes %pK\n", __func__, queue, num_buffers, num_planes, sizes);
		ret = -EINVAL;
		goto exit;
	}

	stream = queue->drv_priv;
	if (!stream || !stream->video_dev) {
		pr_err("%s: invalid params: stream %pk, video_dev %pK\n", __func__,
			stream, stream->video_dev);
		ret = -EINVAL;
		goto exit;
	}

	vvd = to_virtio_vd(stream->video_dev);
	port = v4l2_type_to_driver_port(stream, queue->type, __func__);
	if (port < 0) {
		v4l2_err(&vvd->v4l2_dev, "%s: port not found for v4l2 type %d\n",
			__func__, queue->type);
		ret = -EINVAL;
		goto exit;
	}

	format.type = queue->type;
	ret = virtio_video_cmd_g_fmt(vvd, stream, &format);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: g_fmt failed.\n", __func__);
		goto exit;
	}

	if((port == INPUT_PORT || port == OUTPUT_PORT)) {
		*num_planes = format.fmt.pix_mp.num_planes;
		for (plane = 0; plane < *num_planes; plane++) {
			sizes[plane] = format.fmt.pix_mp.plane_fmt[plane].sizeimage;
			v4l2_info(&vvd->v4l2_dev, "%s: plane %d: size is %d.\n",
			__func__, plane, sizes[plane]);
		}
	}
	else if (port == INPUT_META_PORT || port == OUTPUT_META_PORT) {
		*num_planes = 1;
		sizes[0] = format.fmt.meta.buffersize;
	}
	v4l2_info(&vvd->v4l2_dev, "%s: type %s, num_buffers %d, "
		"*num_planes %d, sizes[0] %d\n", __func__, v4l2_type_name(queue->type),
		*num_buffers, *num_planes, sizes[0]);

exit:
	return ret;
}

int msm_vidc_start_streaming(struct vb2_queue *queue, unsigned int count)
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device* vvd = NULL;

	if (!queue || !queue->drv_priv) {
		pr_err("%s: invalid input, queue %pK, stream %pK\n", __func__,
			queue, queue->drv_priv);
		ret = -EINVAL;
		goto exit;
	}

	stream = queue->drv_priv;
	vvd = to_virtio_vd(stream->video_dev);

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	if (queue->type == INPUT_META_PLANE || queue->type == OUTPUT_META_PLANE) {
		v4l2_info(&vvd->v4l2_dev, "%s: nothing to start on %s\n",
			__func__, v4l2_type_name(queue->type));
		ret = 0;
		goto unlock;
	}

	if (!is_decode_session(vvd) && !is_encode_session(vvd)) {
		v4l2_err(&vvd->v4l2_dev, "%s: invalid session %d\n",
			__func__, vvd->type);
		ret = -EINVAL;
		goto unlock;
	}

	ret = virtio_video_cmd_streamon(vvd, stream, queue->type);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: streamon %s failed\n",
			__func__, v4l2_type_name(queue->type));
	}

unlock:
	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);

exit:
	return ret;
}

void msm_vidc_stop_streaming(struct vb2_queue *queue)
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device* vvd = NULL;

	if (!queue || !queue->drv_priv) {
		pr_err("%s: invalid input, queue %pK, stream %pK\n", __func__,
			queue, queue->drv_priv);
		return;
	}

	stream = queue->drv_priv;
	vvd = to_virtio_vd(stream->video_dev);

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	if (queue->type == INPUT_META_PLANE || queue->type == OUTPUT_META_PLANE) {
		v4l2_info(&vvd->v4l2_dev, "%s: nothing to stop on %s\n",
			__func__, v4l2_type_name(queue->type));
		goto unlock;
	}

	if (!is_decode_session(vvd) && !is_encode_session(vvd)) {
		v4l2_err(&vvd->v4l2_dev, "%s: invalid session %d\n",
			__func__, vvd->type);
		goto unlock;
	}

	ret = virtio_video_cmd_streamoff(vvd, stream, queue->type);
	if (ret) {
		v4l2_err(&vvd->v4l2_dev, "%s: streamoff %s failed\n",
			__func__, v4l2_type_name(queue->type));
	}

unlock:
	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);
}

void msm_vidc_buf_queue(struct vb2_buffer *vb2)
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device* vvd = NULL;
	struct virtio_video_buffer *virtio_vb = to_virtio_vb(vb2);
	int plane = 0;
	__u8 v4l2_buf[sizeof(struct v4l2_buffer)
		+ VIDEO_MAX_PLANES * sizeof(struct v4l2_plane)] = {0};
	struct v4l2_buffer *pb = (struct v4l2_buffer *)v4l2_buf;
	struct vb2_queue *queue = vb2->vb2_queue;
	int export_id[VIDEO_MAX_PLANES] = { 0 };
	u64 buf_fd = 0;

	stream = vb2_get_drv_priv(vb2->vb2_queue);

	if (!stream) {
		pr_err("%s: invalid stream\n", __func__);
		return;
	}

	vvd = to_virtio_vd(stream->video_dev);

	client_lock(stream, __func__);
	inst_lock(stream, __func__);

	if (queue->is_multiplanar) {
		for (plane = 0; plane < vb2->num_planes; plane++) {
			buf_fd = vb2->planes[plane].m.fd;
			export_id[plane] = msm_buf_get_export_id(stream,
								 vvd->commandq.vq->habmm_handle,
								 buf_fd, vb2->planes[plane].length,
								 vb2->type, true);
			v4l2_info(&vvd->v4l2_dev, "%s, export buf fd 0x%x to export_id[%d] 0x%x",
				  __func__, buf_fd, plane, export_id[plane]);
		}

		pb->m.planes = (struct v4l2_plane *)(v4l2_buf + sizeof(struct v4l2_buffer));
		queue->buf_ops->fill_user_buffer(vb2, pb);

		for (plane = 0; plane < vb2->num_planes; plane++) {
			pb->m.planes[plane].m.fd = export_id[plane];
			v4l2_info(&vvd->v4l2_dev, "%s: QBUF: %s: idx %d, memory %d, fd %d=%#x, "
				  "size %d, filled %d, offset %d time-stamp %d", __func__,
				  v4l2_type_name(pb->type), pb->index, pb->memory,
				  pb->m.planes[plane].m.fd, pb->m.planes[plane].m.fd,
				  pb->m.planes[plane].length,
				  pb->m.planes[plane].bytesused,
				  pb->m.planes[plane].data_offset, pb->timestamp);
		}
	} else {
		queue->buf_ops->fill_user_buffer(vb2, pb);

		buf_fd = pb->m.fd;
		export_id[0] = msm_buf_get_export_id(stream, vvd->commandq.vq->habmm_handle,
						     buf_fd, pb->length, pb->type, true);
		pb->m.fd = export_id[0];
		v4l2_info(&vvd->v4l2_dev, "%s: QBUF: %s: idx %d, memory %d, fd %d=%#x, "
			  "size %d, filled %d, time-stamp %d", __func__,
			  v4l2_type_name(pb->type), pb->index, pb->memory,
			  pb->m.fd, pb->m.fd, pb->length, pb->bytesused, pb->timestamp);
	}

	virtio_vb->queued = true;
	virtio_vb->resource_id = export_id[0];

	virtio_video_pending_buf_list_add(vvd, virtio_vb);
	ret = virtio_video_cmd_qbuf(vvd, stream, pb, virtio_vb);
	if (ret) {
		vb2_buffer_done(vb2, VB2_BUF_STATE_ERROR);
	}

	inst_unlock(stream, __func__);
	client_unlock(stream, __func__);
	put_inst(stream);
}

void msm_vidc_buf_cleanup(struct vb2_buffer *vb2)
{
}

int msm_vidc_buf_out_validate(struct vb2_buffer *vb2)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb2);

	vbuf->field = V4L2_FIELD_NONE;

	return 0;
}

void msm_vidc_buf_request_complete(struct vb2_buffer *vb2)
{
	struct virtio_video_stream *stream = vb2_get_drv_priv(vb2->vb2_queue);
	struct virtio_video_device* vvd = to_virtio_vd(stream->video_dev);

	v4l2_info(&vvd->v4l2_dev,  "%s: vb2 type %d, index %d\n",
		  __func__, vb2->type, vb2->index);
	v4l2_ctrl_request_complete(vb2->req_obj.req, &stream->ctrl_handler);
}

int vb2q_init(struct virtio_video_stream *stream,
	struct vb2_queue *queue, enum v4l2_buf_type type,
	struct vb2_ops* ops, const struct vb2_mem_ops* mem_ops)
{
	int ret = 0;
	struct virtio_video_device* vvd = NULL;

	if (!stream || !queue || !stream->video_dev) {
		pr_err("%s: invalid params: stream %pK, queue %pK, video_dev %pK\n",
			__func__, stream, queue, stream->video_dev);
		ret = -EINVAL;
		goto exit;
	}

	vvd = to_virtio_vd(stream->video_dev);

	queue->type = type;
	queue->io_modes = VB2_MMAP | VB2_DMABUF;
	queue->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	queue->ops = ops;
	queue->mem_ops = mem_ops;
	queue->drv_priv = stream;
	queue->allow_zero_bytesused = 1;
	queue->copy_timestamp = 1;

	ret = vb2_queue_init(queue);
	if (ret)
		v4l2_err(&vvd->v4l2_dev, "%s: vb2_queue_init failed for type %d\n",
			__func__, type);

exit:
	return ret;
}
