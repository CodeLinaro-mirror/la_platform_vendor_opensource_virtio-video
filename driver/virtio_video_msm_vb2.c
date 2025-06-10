/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "virtio_video.h"
#include "virtio_video_msm_vb2.h"
#include "virtio_video_msm_vq.h"
#include "virtio_video_msm_debug.h"
#include "virtio_video_msm_mem.h"
#include "virtio_video_msm_hab.h"

struct vb2_queue *msm_vidc_get_vb2q(struct virtio_video_stream* stream,
				    uint32_t type, const char *func)
{
	struct vb2_queue *queue = NULL;
	struct virtio_video_device* vvd = NULL;

	if (!stream) {
		vpr_e(vvd2tag(vvd), "%s: invalid stream\n", func);
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
		vpr_e(strm2tag(stream), "%s: invalid buffer type %d\n",
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

int queue_check_input_params(struct vb2_queue *queue,
			     unsigned int *num_buffers,
			     unsigned int *num_planes,
			     unsigned int sizes[],
			     struct virtio_video_device* vvd,
			     struct virtio_video_stream **stream)
{
	if (!queue || !num_buffers || !num_planes || !sizes) {
		vpr_e(vvd2tag(vvd), "%s: invalid params: queue %pK, num_buffers %pK"
		      ", num_planes %pK sizes %pK\n",
		      __func__, queue, num_buffers, num_planes, sizes);
		return -EINVAL;
	}

	*stream = queue->drv_priv;
	if (!*stream) {
		vpr_e(vvd2tag(vvd), "%s: invalid params: stream %pk\n",
		      __func__, *stream);
		return -EINVAL;
	}

	if (!(*stream)->video_dev) {
		vpr_e(vvd2tag(vvd), "%s: invalid params: video_dev %pK\n",
		      __func__, (*stream)->video_dev);
		return -EINVAL;
	}

	if (is_session_error(*stream)) {
		vpr_e(strm2tag(*stream),"%s: in session error state\n", __func__);
		return -EIO;
	}

	return 0;
}

void queue_set_size(struct virtio_video_stream *stream,
		    unsigned int *num_planes, unsigned int sizes[],
		    int port, struct v4l2_format* format)
{
	int plane = 0;
	if((port == INPUT_PORT || port == OUTPUT_PORT)) {
		*num_planes = format->fmt.pix_mp.num_planes;
		for (plane = 0; plane < *num_planes; plane++) {
			sizes[plane] = format->fmt.pix_mp.plane_fmt[plane].sizeimage;
			vpr_h(strm2tag(stream), "%s: plane %d: size is %d.\n",
			      __func__, plane, sizes[plane]);
		}
	}
	else if (port == INPUT_META_PORT || port == OUTPUT_META_PORT) {
		*num_planes = 1;
		sizes[0] = format->fmt.meta.buffersize;
	}
}

int msm_vidc_queue_setup(struct vb2_queue *queue,
			 unsigned int *num_buffers, unsigned int *num_planes,
			 unsigned int sizes[], struct device *alloc_devs[])
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device* vvd = NULL;
	int port = 0;
	struct v4l2_format format = {0};

	ret = queue_check_input_params(queue, num_buffers, num_planes, sizes,
				       vvd, &stream);

	if (ret)
		return ret;

	vvd = to_virtio_vd(stream->video_dev);
	port = v4l2_type_to_driver_port(stream, queue->type, __func__);
	if (port < 0) {
		vpr_e(strm2tag(stream), "%s: port not found for v4l2 type %d\n",
		      __func__, queue->type);
		ret = -EINVAL;
		return ret;
	}

	format.type = queue->type;
	ret = virtio_video_cmd_g_fmt(vvd, stream, &format);
	if (ret) {
		vpr_e(strm2tag(stream), "%s: g_fmt failed.\n", __func__);
		return ret;
	}

	queue_set_size(stream, num_planes, sizes, port, &format);
	vpr_h(strm2tag(stream), "%s: type %s, num_buffers %d, "
	      "*num_planes %d, sizes[0] %d\n", __func__, v4l2_type_name(queue->type),
	      *num_buffers, *num_planes, sizes[0]);

	return ret;
}

int msm_vidc_start_streaming(struct vb2_queue *queue, unsigned int count)
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device* vvd = NULL;

	if (!queue) {
		vpr_e(vvd2tag(vvd), "%s: invalid input, queue %pK\n",
		      __func__, queue);
		ret = -EINVAL;
		goto exit;
	}

	if (!queue->drv_priv) {
		vpr_e(vvd2tag(vvd), "%s: invalid input, stream %pK\n",
		      __func__, queue->drv_priv);
		ret = -EINVAL;
		goto exit;
	}

	stream = queue->drv_priv;
	vvd = to_virtio_vd(stream->video_dev);

	if (is_session_error(stream)) {
		vpr_e(strm2tag(stream),"%s: in session error state\n", __func__);
		ret = -EIO;
		goto exit;
	}

	if (queue->type == INPUT_META_PLANE || queue->type == OUTPUT_META_PLANE) {
		vpr_h(strm2tag(stream), "%s: nothing to start on %s\n",
		      __func__, v4l2_type_name(queue->type));
		ret = 0;
		goto exit;
	}

	if (!is_decode_session(vvd) && !is_encode_session(vvd)) {
		vpr_e(strm2tag(stream), "%s: invalid session %d\n",
		      __func__, vvd->type);
		ret = -EINVAL;
		goto exit;
	}

	ret = virtio_video_cmd_streamon(vvd, stream, queue->type);
	if (ret) {
		vpr_e(strm2tag(stream), "%s: streamon %s failed\n",
		      __func__, v4l2_type_name(queue->type));
	}

exit:
	return ret;
}

void buf_put_export_queue(struct vb2_queue *queue)
{
	struct virtio_video_stream *stream = queue->drv_priv;

	if (queue->type == OUTPUT_MPLANE) {
		vpr_h(strm2tag(stream), "%s: OUTPUT unexport\n", __func__);
		msm_buf_put_export_queue_type(stream, VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT);
		vpr_h(strm2tag(stream), "%s: OUTMTA unexport\n", __func__);
		msm_buf_put_export_queue_type(stream, VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT_META);
	} else if (queue->type == INPUT_MPLANE) {
		vpr_h(strm2tag(stream), "%s:  INPUT unexport\n", __func__);
		msm_buf_put_export_queue_type(stream, VIRTIO_VIDEO_QUEUE_TYPE_INPUT);
		vpr_h(strm2tag(stream), "%s:  INMTA unexport\n", __func__);
		msm_buf_put_export_queue_type(stream, VIRTIO_VIDEO_QUEUE_TYPE_INPUT_META);
	}
}

void queue_release_buffers(struct vb2_queue *queue)
{
	struct virtio_video_stream *stream = queue->drv_priv;

	if (queue->type == INPUT_MPLANE)
		virtio_video_queue_release_buffers(stream,
			               VIRTIO_VIDEO_QUEUE_TYPE_INPUT_META);
	else if (queue->type == OUTPUT_MPLANE)
		virtio_video_queue_release_buffers(stream,
			               VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT_META);
}

void msm_vidc_stop_streaming(struct vb2_queue *queue)
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device* vvd = NULL;
	enum virtio_video_queue_type virtio_queue_type = 0;

	if (!queue) {
		vpr_e(vvd2tag(vvd), "%s: invalid input, queue %pK\n",
		      __func__, queue);
		ret = -EINVAL;
		goto exit;
	}

	if (!queue->drv_priv) {
		vpr_e(vvd2tag(vvd), "%s: invalid input, stream %pK\n",
		      __func__, queue->drv_priv);
		ret = -EINVAL;
		goto exit;
	}

	stream = queue->drv_priv;
	vvd = to_virtio_vd(stream->video_dev);
	virtio_queue_type = to_virtio_queue_type(queue->type);

	if (!is_decode_session(vvd) && !is_encode_session(vvd)) {
		vpr_e(strm2tag(stream), "%s: unsupported session type %d\n",
		      __func__, vvd->type);
		goto exit;
	}

	if (is_session_error(stream)) {
		vpr_e(strm2tag(stream),"%s: %s in session error state\n",
		      __func__, v4l2_type_name(queue->type));
		ret = -EIO;
		goto session_err;
	}

	if (queue->type == INPUT_META_PLANE || queue->type == OUTPUT_META_PLANE)
		goto wait_vb2;

	ret = virtio_video_cmd_streamoff(vvd, stream, queue->type);
	buf_put_export_queue(queue);

session_err:
	if (ret) {
		vpr_h(strm2tag(stream), "%s: streamoff %s failed %d, to release buffers\n",
		      __func__, v4l2_type_name(queue->type), ret);

		queue_release_buffers(queue);
		virtio_video_queue_release_buffers(stream, virtio_queue_type);
	}

wait_vb2:
	ret = vb2_wait_for_all_buffers(queue);
	if (ret)
		vpr_e(strm2tag(stream), "%s: failed for waitting %s buffer done %d\n",
		      __func__, v4l2_type_name(queue->type), ret);
	else
		vpr_h(strm2tag(stream), "%s: %s done\n", __func__,
			v4l2_type_name(queue->type));

exit:

	return;
}

void msm_vidc_buf_queue(struct vb2_buffer *vb2)
{
	int ret = 0;
	struct virtio_video_stream *stream = NULL;
	struct virtio_video_device *vvd = NULL;
	struct hab_virtqueue *hvq = NULL;
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
		vpr_e(vvd2tag(vvd), "%s: invalid stream\n", __func__);
		return;
	}

	if (is_session_error(stream)) {
		vpr_e(strm2tag(stream),"%s: in session error state\n", __func__);
		ret = -1;
		goto de_active_vb2;
	}

	vvd = to_virtio_vd(stream->video_dev);

	hvq = to_hab_vq(vvd->commandq.vq);

	if (queue->is_multiplanar) {
		for (plane = 0; plane < vb2->num_planes; plane++) {
			buf_fd = vb2->planes[plane].m.fd;
			export_id[plane]
				= msm_buf_get_export_id(stream, buf_fd,
				                        vb2->planes[plane].length,
				                        to_virtio_queue_type(vb2->type));
			if (!export_id[plane]) {
				ret = -1;
				goto unexport;
			}
			vpr_l(strm2tag(stream), "%s, export buf_fd %d to export_id[%d] %d vb2 %#p\n",
			      __func__, buf_fd, plane, export_id[plane], vb2);
		}

		pb->m.planes = (struct v4l2_plane *)(v4l2_buf + sizeof(struct v4l2_buffer));
		queue->buf_ops->fill_user_buffer(vb2, pb);

		for (plane = 0; plane < vb2->num_planes; plane++) {
			pb->m.planes[plane].m.fd = export_id[plane];
			vpr_h(strm2tag(stream), "%s: QBUF: %s: idx %3d fd %3d=%#3x "
			      "size %8d filled %8d offset %d ts %llu vb2 %p\n", __func__,
			      v4l2_type_name(pb->type), pb->index,
			      pb->m.planes[plane].m.fd,
			      pb->m.planes[plane].m.fd,
			      pb->m.planes[plane].length,
			      pb->m.planes[plane].bytesused,
			      pb->m.planes[plane].data_offset,
			      v4l2_buffer_get_timestamp(pb), vb2);
		}
	} else {
		queue->buf_ops->fill_user_buffer(vb2, pb);

		buf_fd = pb->m.fd;
		export_id[0] = msm_buf_get_export_id(stream, buf_fd, pb->length,
		                                     to_virtio_queue_type(pb->type));
		if (!export_id[0]) {
			ret = -1;
			goto unexport;
		}
		pb->m.fd = export_id[0];
		vpr_h(strm2tag(stream), "%s: QBUF: %s: idx %3d fd %3d=%#3x "
		      "size %8d filled %8d ts %llu vb2 %p\n", __func__,
		      v4l2_type_name(pb->type), pb->index, pb->m.fd, pb->m.fd,
		      pb->length, pb->bytesused,
		      v4l2_buffer_get_timestamp(pb), vb2);
	}

	virtio_vb->queued = true;
	virtio_vb->resource_id = export_id[0];

	virtio_video_pending_buf_list_add(vvd, virtio_vb);
	ret = virtio_video_cmd_qbuf(vvd, stream, pb, virtio_vb);

unexport:
	if (ret) {
		for (plane = 0; plane < VIDEO_MAX_PLANES; plane++) {
			if (export_id[plane])
				msm_buf_put_export_id(stream, export_id[plane],
				                      to_virtio_queue_type(vb2->type), true);
		}
	}

de_active_vb2:
	if (ret)
		vb2_buffer_done(vb2, VB2_BUF_STATE_ERROR);
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

	vpr_h(strm2tag(stream),  "%s: vb2 type %d, index %d\n",
	      __func__, vb2->type, vb2->index);
	v4l2_ctrl_request_complete(vb2->req_obj.req, &stream->ctrl_handler);
}

int vb2q_init(struct virtio_video_stream *stream,
	struct vb2_queue *queue, enum v4l2_buf_type type,
	struct vb2_ops* ops, const struct vb2_mem_ops* mem_ops)
{
	int ret = 0;
	struct virtio_video_device* vvd = NULL;

	if (!stream || !queue) {
		vpr_e(vvd2tag(vvd), "%s: invalid params: stream %pK, queue %pK\n",
		      __func__, stream, queue);
		ret = -EINVAL;
		return ret;
	}

	if (!stream->video_dev) {
		vpr_e(vvd2tag(vvd), "%s: invalid params: video_dev %pK\n",
		      __func__, stream->video_dev);
		ret = -EINVAL;
		return ret;
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
	queue->buf_struct_size = sizeof(struct virtio_video_buffer);

	ret = vb2_queue_init(queue);
	if (ret)
		vpr_e(strm2tag(stream), "%s: vb2_queue_init failed for type %d\n",
		      __func__, type);

	return ret;
}
