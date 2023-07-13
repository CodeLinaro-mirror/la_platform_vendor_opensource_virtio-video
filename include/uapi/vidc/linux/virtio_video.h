/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * Virtio Video Device
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (C) 2020 OpenSynergy GmbH.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _UAPI_LINUX_VIRTIO_VIDEO_H
#define _UAPI_LINUX_VIRTIO_VIDEO_H

#include <linux/types.h>
#include <linux/virtio_config.h>

#define MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE                   (1024)
#define VIRTIO_VIDEO_BUF_FLAG_KEYFRAME                      0x0008
#define VIRTIO_VIDEO_BUF_FLAG_LAST                          0x00100000
#define VIRTIO_VIDEO_BUF_FLAG_CODECCONFIG                   0x01000000
#define VIRTIO_VIDEO_BUF_FLAG_END_OF_SUBFRAME               0x02000000
#define VIRTIO_VIDEO_BUF_FLAG_DATA_CORRUPT                  0x04000000
#define VIRTIO_VIDEO_BUF_INPUT_UNSUPPORTED                  0x08000000
#define VIRTIO_VIDEO_BUF_FLAG_EOS                           0x10000000
#define VIRTIO_VIDEO_BUF_FLAG_READONLY                      0x20000000
#define VIRTIO_VIDEO_BUF_FLAG_PERF_MODE                     0x40000000
#define VIRTIO_VIDEO_BUF_FLAG_CVPMETADATA_SKIP              0x80000000
#define VIRTIO_VIDEO_EVENT_PRIVATE_START                    0x08000000
#define VIRTIO_VIDEO_EVENT_MSM_VIDC_START \
        (VIRTIO_VIDEO_EVENT_PRIVATE_START + 0x00001000)
#define VIRTIO_VIDEO_EVENT_MSM_VIDC_SYS_ERROR \
        (VIRTIO_VIDEO_EVENT_MSM_VIDC_START + 5)
 /* Guest pages can be used for video buffers. */
#define VIRTIO_VIDEO_F_RESOURCE_GUEST_PAGES                 0
#define VIRTIO_VIDEO_FMT_FLAG_COMPRESSED                    0x0001
/*
 * The host can process buffers even if they are non-contiguous memory such as
 * scatter-gather lists.
 */
#define VIRTIO_VIDEO_F_RESOURCE_NON_CONTIG                  1
#define VIRTIO_VIDEO_F_VENDOR                               2
#define VIRTIO_VIDEO_MAX_PLANES                             8

/*  Four-character-code (FOURCC) */
#define virtio_video_fourcc(a, b, c, d)\
	((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))
#define virtio_video_fourcc_be(a, b, c, d)  \
	(virtio_video_fourcc(a, b, c, d) | (1 << 31))
/*NV12*/
#define VIRTIO_VIDEO_PIX_FMT_NV12 \
	virtio_video_fourcc('N', 'V', '1', '2')
/*NV12C*/
#define VIRTIO_VIDEO_PIX_FMT_VIDC_NV12C \
	virtio_video_fourcc('Q', '1', '2', 'C')
/*NV21*/
#define VIRTIO_VIDEO_PIX_FMT_NV21 \
	virtio_video_fourcc('N', 'V', '2', '1')
/*RGBA32*/
#define VIRTIO_VIDEO_PIX_FMT_RGBA32 \
	virtio_video_fourcc('A', 'B', '2', '4')
/*TP10C*/
#define VIRTIO_VIDEO_PIX_FMT_VIDC_TP10C \
	virtio_video_fourcc('Q', '1', '0', 'C')
/*ARGB32C*/
#define VIRTIO_VIDEO_PIX_FMT_VIDC_ARGB32C \
	virtio_video_fourcc('Q', '2', '4', 'C')
/*P010*/
#define VIRTIO_VIDEO_PIX_FMT_VIDC_P010 \
	virtio_video_fourcc('P', '0', '1', '0')
 /* compressed formats */
#define VIRTIO_VIDEO_MSM_PIX_FMT_H264 \
	virtio_video_fourcc('H', '2', '6', '4') /* H264 with start codes */
#define VIRTIO_VIDEO_MSM_PIX_FMT_MPEG2 \
	virtio_video_fourcc('M', 'P', 'G', '2') /* MPEG-2 ES     */
#define VIRTIO_VIDEO_MSM_PIX_FMT_MPEG4 \
	virtio_video_fourcc('M', 'P', 'G', '4') /* MPEG-4 part 2 ES */
#define VIRTIO_VIDEO_MSM_PIX_FMT_VP9 \
	virtio_video_fourcc('V', 'P', '9', '0') /* VP9 */
#define VIRTIO_VIDEO_MSM_PIX_FMT_HEVC \
	virtio_video_fourcc('H', 'E', 'V', 'C') /* for HEVC stream */
#define VIRTIO_VIDEO_CTRL_CLASS_USER        0x00980000    /* Old-style 'user' controls */
#define VIRTIO_VIDEO_CTRL_CLASS_MPEG        0x00990000    /* MPEG-compression controls */
#define VIRTIO_VIDEO_CID_MPEG_BASE (VIRTIO_VIDEO_CTRL_CLASS_MPEG | 0x900)
#define VIRTIO_VIDEO_CID_MPEG_CLASS (VIRTIO_VIDEO_CTRL_CLASS_MPEG | 1)
#define VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE (VIRTIO_VIDEO_CTRL_CLASS_MPEG | 0x2000)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_B_FRAMES (VIRTIO_VIDEO_CID_MPEG_BASE + 202)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES (VIRTIO_VIDEO_CID_MPEG_BASE + 219)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB (VIRTIO_VIDEO_CID_MPEG_BASE + 220)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MULTI_SLICE_MODE (VIRTIO_VIDEO_CID_MPEG_BASE + 221)
#define VIRTIO_VIDEO_CID_BASE (VIRTIO_VIDEO_CTRL_CLASS_USER | 0x900)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_PROFILE (VIRTIO_VIDEO_CID_MPEG_BASE + 363)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_LOWLATENCY_REQUEST (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 3)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_CODEC_CONFIG (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 4)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_RANDOM \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 13)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VIDC_INTRA_REFRESH_TYPE \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0xC)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VENC_COMPLEXITY           (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 167)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VBV_DELAY (VIRTIO_VIDEO_CID_MPEG_BASE + 225)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_LTR_COUNT (VIRTIO_VIDEO_CID_MPEG_BASE + 232)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_USE_LTR_FRAMES (VIRTIO_VIDEO_CID_MPEG_BASE + 234)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_FRAME_LTR_INDEX (VIRTIO_VIDEO_CID_MPEG_BASE + 233)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_I_FRAME_MIN_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 385)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_I_FRAME_MAX_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 386)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_FRAME_RATE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0x5)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_B_FRAME_MIN_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 389)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_B_FRAME_MAX_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 390)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_P_FRAME_MIN_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 387)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_P_FRAME_MAX_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 388)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_CONSTANT_QUALITY (VIRTIO_VIDEO_CID_MPEG_BASE + 645)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_I_FRAME_MIN_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 647)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_I_FRAME_MAX_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 648)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_P_FRAME_MIN_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 649)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_P_FRAME_MAX_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 650)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_B_FRAME_MIN_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 651)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_B_FRAME_MAX_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 652)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB (VIRTIO_VIDEO_CID_MPEG_BASE + 214)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE (VIRTIO_VIDEO_CID_MPEG_BASE + 362)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA (VIRTIO_VIDEO_CID_MPEG_BASE + 360)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA (VIRTIO_VIDEO_CID_MPEG_BASE + 361)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_FORCE_KEY_FRAME (VIRTIO_VIDEO_CID_MPEG_BASE + 229)
#define VIRTIO_VIDEO_CID_ROTATE (VIRTIO_VIDEO_CID_BASE + 34)
#define VIRTIO_VIDEO_CID_HFLIP (VIRTIO_VIDEO_CID_BASE + 20)
#define VIRTIO_VIDEO_CID_VFLIP (VIRTIO_VIDEO_CID_BASE + 21)
#define VIRTIO_VIDEO_CID_BACKLIGHT_COMPENSATION (VIRTIO_VIDEO_CID_BASE + 28)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_OPERATING_RATE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 74)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_EXTRADATA (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 25)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_SECURE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 1)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_LOWLATENCY_MODE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 56)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_USELTRFRAME (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 49)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_MARKLTRFRAME (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 50)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_LTRCOUNT (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 48)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_VUI_TIMING_INFO \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 19)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_TYPE (VIRTIO_VIDEO_CID_MPEG_BASE + 606)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_LAYER (VIRTIO_VIDEO_CID_MPEG_BASE + 607)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_COMPRESSION_QUALITY \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 118)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_HEVC_MAX_HIER_CODING_LAYER \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 120)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_BASELAYER_ID (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 77)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_VPX_ERROR_RESILIENCE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 63)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_PRIORITY (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0x2A)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_CHROMA_QP_INDEX_OFFSET  (VIRTIO_VIDEO_CID_MPEG_BASE+384)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEADER_MODE (VIRTIO_VIDEO_CID_MPEG_BASE+216)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_L0_BR (VIRTIO_VIDEO_CID_MPEG_BASE + 636)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_L1_BR    (VIRTIO_VIDEO_CID_MPEG_BASE + 637)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_L2_BR    (VIRTIO_VIDEO_CID_MPEG_BASE + 638)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_L3_BR    (VIRTIO_VIDEO_CID_MPEG_BASE + 639)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_L4_BR    (VIRTIO_VIDEO_CID_MPEG_BASE + 640)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_L5_BR    (VIRTIO_VIDEO_CID_MPEG_BASE + 641)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_HIER_CODING_L6_BR    (VIRTIO_VIDEO_CID_MPEG_BASE + 642)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VENC_BITRATE_BOOST \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 132)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_INTERLACE   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 23)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_DPB_LUMA_CHROMA_MISR   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 21)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_OPB_LUMA_CHROMA_MISR   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 22)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_CONCEALED_MB_COUNT   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 24)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_HISTOGRAM_INFO   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 25)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_SEI_MASTERING_DISPLAY_COLOUR   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 26)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_SEI_CONTENT_LIGHT_LEVEL   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 27)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_HDR10PLUS   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 28)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_BUFFER_TAG   (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 30)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_SEQ_HEADER_NAL       (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0x14)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_METADATA_SUBFRAME_OUTPUT      (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0x1F)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_FRAME_RATE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 8)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_8X8_TRANSFORM (VIRTIO_VIDEO_CID_MPEG_BASE+355)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_SIZE_OF_LENGTH_FIELD (VIRTIO_VIDEO_CID_MPEG_BASE + 635)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VENC_NATIVE_RECORDER \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 122)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_COLOR_SPACE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 93)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_FULL_RANGE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 94)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_TRANSFER_CHARS (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 95)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_MATRIX_COEFFS (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 96)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_VPE_CSC (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 87)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_VPE_CSC_CUSTOM_MATRIX \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 114)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_BLUR_DIMENSIONS \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 57)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VENC_BITRATE_SAVINGS \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 131)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_CLIENT_ID \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 65)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_CONTENT_ADAPTIVE_CODING                            \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0xE)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_MIN_BITSTREAM_SIZE_OVERWRITE \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0x23)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_LAST_FLAG_EVENT_ENABLE                             \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 0x42)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_BUFFER_SIZE_LIMIT (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 64)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE (VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 23)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_DECODE_ORDER \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE+3)
#define VIRTIO_VIDEO_MPEG_MSM_VIDC_DISABLE 0
#define VIRTIO_VIDEO_MPEG_MSM_VIDC_ENABLE 1
#define VIRTIO_VIDEOFE_METADATA_DEFAULT_PORTINDEX_INPUT      0
#define VIRTIO_VIDEOFE_METADATA_DEFAULT_PORTINDEX_OUTPUT     1
#define VIRTIO_VIDEOFE_METADATA_DEFAULT_VERSION              0x01010200
#define virtio_video_fourcc(a, b, c, d)  ((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))
#define VIRTIO_VIDEO_PIX_FMT_H264 virtio_video_fourcc('H', '2', '6', '4')
#define VIRTIO_VIDEO_PIX_FMT_HEVC virtio_video_fourcc('H', 'E', 'V', 'C')
#define VIRTIO_VIDEO_PIX_FMT_VP9 virtio_video_fourcc('V', 'P', '9', '0')
#define VIRTIO_VIDEO_PIX_FMT_MPEG2 virtio_video_fourcc('M', 'P', 'G', '2')
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_DISABLE_TIMESTAMP_REORDER \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 136)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_LEVEL            (VIRTIO_VIDEO_CID_MPEG_BASE+359)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_PROFILE    (VIRTIO_VIDEO_CID_MPEG_BASE + 615)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_LEVEL        (VIRTIO_VIDEO_CID_MPEG_BASE + 616)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MPEG2_PROFILE        (VIRTIO_VIDEO_CID_MPEG_BASE+271)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MPEG2_LEVEL            (VIRTIO_VIDEO_CID_MPEG_BASE+270)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VP9_PROFILE            (VIRTIO_VIDEO_CID_MPEG_BASE+512)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VP9_LEVEL            (VIRTIO_VIDEO_CID_MPEG_BASE+513)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_VP9_LEVEL \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 67)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_MPEG2_PROFILE \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE+24)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_MPEG2_LEVEL \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE+23)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_TIER            (VIRTIO_VIDEO_CID_MPEG_BASE + 618)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MPEG4_LEVEL        (VIRTIO_VIDEO_CID_MPEG_BASE+405)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MPEG4_PROFILE    (VIRTIO_VIDEO_CID_MPEG_BASE+406)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_I_FRAME_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 350)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_P_FRAME_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 351)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_B_FRAME_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 352)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_FRAME_RC_ENABLE            (VIRTIO_VIDEO_CID_MPEG_BASE+215)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_BITRATE_MODE     (VIRTIO_VIDEO_CID_MPEG_BASE+206)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_BITRATE         (VIRTIO_VIDEO_CID_MPEG_BASE+207)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_BITRATE_PEAK     (VIRTIO_VIDEO_CID_MPEG_BASE+208)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_GOP_SIZE         (VIRTIO_VIDEO_CID_MPEG_BASE+203)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_ENTROPY_MODE        (VIRTIO_VIDEO_CID_MPEG_BASE+357)
#define VIRTIO_VIDEO_CID_MIN_BUFFERS_FOR_CAPTURE    (VIRTIO_VIDEO_CID_BASE + 39)
#define VIRTIO_VIDEO_CID_MIN_BUFFERS_FOR_OUTPUT     (VIRTIO_VIDEO_CID_BASE + 40)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_STREAM_FORMAT \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE+2)
// Define VIRTIO_VIDEO_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR control code if not present in header files.
#ifndef VIRTIO_VIDEO_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR (VIRTIO_VIDEO_CID_MPEG_BASE + 644)
#endif
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_ROI_TYPE \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 128)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_IMG_GRID_SIZE \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 117)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VPX_MIN_QP    (VIRTIO_VIDEO_CID_MPEG_BASE + 507)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VPX_MAX_QP    (VIRTIO_VIDEO_CID_MPEG_BASE + 508)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VPX_I_FRAME_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 509)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_VPX_P_FRAME_QP (VIRTIO_VIDEO_CID_MPEG_BASE + 510)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_MIN_QP  (VIRTIO_VIDEO_CID_MPEG_BASE + 353)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_H264_MAX_QP  (VIRTIO_VIDEO_CID_MPEG_BASE + 354)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_MIN_QP  (VIRTIO_VIDEO_CID_MPEG_BASE + 600)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_MAX_QP  (VIRTIO_VIDEO_CID_MPEG_BASE + 601)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_I_FRAME_QP    (VIRTIO_VIDEO_CID_MPEG_BASE + 602)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_P_FRAME_QP    (VIRTIO_VIDEO_CID_MPEG_BASE + 603)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_HEVC_B_FRAME_QP    (VIRTIO_VIDEO_CID_MPEG_BASE + 604)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP  (VIRTIO_VIDEO_CID_MPEG_BASE + 400)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP  (VIRTIO_VIDEO_CID_MPEG_BASE + 401)
#define VIRTIO_VIDEO_CID_MPEG_VIDEO_MPEG4_B_FRAME_QP   (VIRTIO_VIDEO_CID_MPEG_BASE + 402)
#define VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_AU_DELIMITER \
	(VIRTIO_VIDEO_CID_MPEG_MSM_VIDC_BASE + 14)
 /* Current cropping area */
#define VIRTIO_VIDEO_SEL_TGT_CROP       0x0000

/*========================================================================
 Defines structure
========================================================================*/
enum virtio_video_device_type {
	VIRTIO_VIDEO_DEVICE_ENCODER = 0x0100,
	VIRTIO_VIDEO_DEVICE_DECODER,
	VIRTIO_VIDEO_DEVICE_CAMERA,
};

struct virtio_video_event_subscription
{
	__u32               type;
	__u32               id;
	__u32               flags;
	__u32               padding[5];
};

/**
  * struct virtio_video_capability - Describes virtio video device caps returned by VIDIOC_QUERYCAP
  *
  * @driver:       name of the driver module (e.g. "bttv")
  * @card:       name of the card (e.g. "Hauppauge WinTV")
  * @bus_info:       name of the bus (e.g. "PCI:" + pci_name(pci_dev) )
  * @version:       KERNEL_VERSION
  * @capabilities: capabilities of the physical device as a whole
  * @device_caps:  capabilities accessed via this particular device (node)
  * @padding:       padding fields for future extensions
  */
struct virtio_video_capability {
	__u8    driver[16];
	__u8    card[32];
	__u8    bus_info[32];
	__u32   version;
	__u32   capabilities;
	__u32   device_caps;
	__u32   padding[3];
};

enum virtio_video_buf_type {
	VIRTIO_VIDEO_BUF_TYPE_VIDEO_CAPTURE = 1,
	VIRTIO_VIDEO_BUF_TYPE_VIDEO_OUTPUT = 2,
	VIRTIO_VIDEO_BUF_TYPE_VIDEO_CAPTURE_MPLANE = 9,
	VIRTIO_VIDEO_BUF_TYPE_VIDEO_OUTPUT_MPLANE = 10,
	VIRTIO_VIDEO_BUF_TYPE_META_CAPTURE = 13,
	VIRTIO_VIDEO_BUF_TYPE_META_OUTPUT = 14,
	/* Deprecated, do not use */
	VIRTIO_VIDEO_BUF_TYPE_PRIVATE = 0x80,
};

#define BUF_TYPE_IS_MULTIPLANAR(type)            \
	((type) == VIRTIO_VIDEO_BUF_TYPE_VIDEO_CAPTURE_MPLANE    \
	|| (type) == VIRTIO_VIDEO_BUF_TYPE_VIDEO_OUTPUT_MPLANE)

#define BUF_TYPE_IS_OUTPUT(type)            \
	((type) == VIRTIO_VIDEO_BUF_TYPE_VIDEO_CAPTURE_MPLANE    \
	|| (type) == VIRTIO_VIDEO_BUF_TYPE_META_CAPTURE)

#define BUF_TYPE_IS_INPUT(type)            \
	((type) == VIRTIO_VIDEO_BUF_TYPE_VIDEO_OUTPUT_MPLANE    \
	|| (type) == VIRTIO_VIDEO_BUF_TYPE_META_OUTPUT)

#define BUF_TYPE_IS_META(type)            \
	((type) == VIRTIO_VIDEO_BUF_TYPE_META_CAPTURE    \
	|| (type) == VIRTIO_VIDEO_BUF_TYPE_META_OUTPUT)
/*
 *    F O R M A T   E N U M E R A T I O N
 */
struct virtio_video_fmtdesc {
	__u32           index;             /* Format number      */
	__u32           type;              /* enum VIRTIO_VIDEO_buf_type */
	__u32           flags;
	__u8            description[32];   /* Description string */
	__u32           pixelformat;       /* Format fourcc      */
	__u32           padding[4];
};

struct virtio_video_pix_format {
	__u32 width;
	__u32 height;
	__u32 pixelformat;
	__u32 field;
	__u32 bytesperline;
	__u32 sizeimage;
	__u32 colorspace;
	__u32 priv;
	__u32 flags;
	union {
		__u32 ycbcr_enc;
		__u32 hsv_enc;
		};
	__u32 quantization;
	__u32 xfer_func;
};

struct virtio_video_rect {
	__s32   left;
	__s32   top;
	__s32   width;
	__s32   height;
};

/**
 * struct virtio_video_plane_pix_format - additional, per-plane format definition
 * @sizeimage:        maximum size in bytes required for data, for which
 *            this plane will be used
 * @bytesperline:    distance in bytes between the leftmost pixels in two
 *            adjacent lines
 */
struct virtio_video_plane_pix_format {
	__u32        sizeimage;
	__u32        bytesperline;
	__u16        padding[6];
} __attribute__((packed));

/**
 * struct virtio_video_pix_format_mplane - multiplanar format definition
 * @width:        image width in pixels
 * @height:        image height in pixels
 * @pixelformat:    little endian four character code (fourcc)
 * @field:        enum virtio_video_field; field order (for interlaced video)
 * @colorspace:        enum virtio_video_colorspace; supplemental to pixelformat
 * @plane_fmt:        per-plane information
 * @num_planes:        number of planes for this format
 */
struct virtio_video_pix_format_mplane {
	__u32                                   width;
	__u32                                   height;
	__u32                                   pixelformat;
	__u32                                   field;
	__u32                                   colorspace;
	struct virtio_video_plane_pix_format    plane_fmt[VIRTIO_VIDEO_MAX_PLANES];
	__u8                                    num_planes;
	__u8                                    padding[11];
} __attribute__((packed));

/**
 * struct v4l2_meta_format - metadata format definition
 * @dataformat:        little endian four character code (fourcc)
 * @buffersize:        maximum size in bytes required for data
 */
struct virtio_video_meta_format {
	__u32                dataformat;
	__u32                buffersize;
} __attribute__((packed));

/**
 * struct virtio_video_data_format - stream data format
 * @type:    enum virtio_video_buf_type; type of the data stream
 * @pix:    definition of an image format
 * @pix_mp:    definition of a multiplanar image format
 * @win:    definition of an overlaid image
 * @vbi:    raw VBI capture or output parameters
 * @sliced:    sliced VBI capture or output parameters
 * @raw_data:    placeholder for future extensions and custom formats
 */
struct virtio_video_data_format {
	__u32     type;
        __u32     padding;
	union {
		struct virtio_video_pix_format           pix;     /* VIRTIO_VIDEO_BUF_TYPE_VIDEO_CAPTURE */
		struct virtio_video_pix_format_mplane    pix_mp;  /* VIRTIO_VIDEO_BUF_TYPE_VIDEO_CAPTURE_MPLANE */
		struct virtio_video_meta_format          meta;    /*VIRTIO_VIDEO_BUF_TYPE_META_CAPTURE */
		__u8                                     raw_data[200];                   /* user-defined */
		} fmt;
};

/*
 *    F R A M E   S I Z E   E N U M E R A T I O N
 */
enum virtio_video_frmsizetypes {
	VIRTIO_VIDEO_FRMSIZE_TYPE_DISCRETE = 1,
	VIRTIO_VIDEO_FRMSIZE_TYPE_CONTINUOUS = 2,
	VIRTIO_VIDEO_FRMSIZE_TYPE_STEPWISE = 3,
};

struct virtio_video_frmsize_discrete {
	__u32            width;        /* Frame width [pixel] */
	__u32            height;        /* Frame height [pixel] */
};

struct virtio_video_frmsize_stepwise {
	__u32            min_width;    /* Minimum frame width [pixel] */
	__u32            max_width;    /* Maximum frame width [pixel] */
	__u32            step_width;    /* Frame width step size [pixel] */
	__u32            min_height;    /* Minimum frame height [pixel] */
	__u32            max_height;    /* Maximum frame height [pixel] */
	__u32            step_height;    /* Frame height step size [pixel] */
};

struct virtio_video_frmsizeenum {
	__u32            index;        /* Frame size number */
	__u32            pixel_format;    /* Pixel format */
	__u32            type;        /* Frame size type the device supports. */
	union {                    /* Frame size */
		struct virtio_video_frmsize_discrete    discrete;
		struct virtio_video_frmsize_stepwise    stepwise;
	};
	__u32   padding[2];            /* Reserved space for future use */
};

enum virtio_video_mpeg_video_multi_slice_mode {
	VIRTIO_VIDEO_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_BYTES = 2,
	/* Kept for backwards compatibility reasons. Stupid typo... */
	VIRTIO_VIDEO_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES = 2,
};

enum virtio_video_mpeg_video_h264_loop_filter_mode {
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LOOP_FILTER_MODE_ENABLED = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY = 2,
};

enum virtio_video_mpeg_video_hevc_hier_coding_type {
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_B = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_P = 1,
};

enum virtio_video_mpeg_vidc_video_bitrate_savings_type {
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_BRS_DISABLE = 0,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_BRS_ENABLE_8BIT = 1,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_BRS_ENABLE_10BIT = 2,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_BRS_ENABLE_ALL = 3,
};

enum virtio_video_mpeg_video_h264_profile {
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_BASELINE = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_MAIN = 2,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_EXTENDED = 3,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_HIGH = 4,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_HIGH_10 = 5,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_HIGH_422 = 6,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE = 7,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_HIGH_10_INTRA = 8,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_HIGH_422_INTRA = 9,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_HIGH_444_INTRA = 10,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_CAVLC_444_INTRA = 11,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_SCALABLE_BASELINE = 12,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH = 13,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH_INTRA = 14,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_STEREO_HIGH = 15,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_MULTIVIEW_HIGH = 16,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_HIGH = 17,
};

enum virtio_video_mpeg_video_h264_level {
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_1_0 = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_1B = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_1_1 = 2,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_1_2 = 3,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_1_3 = 4,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_2_0 = 5,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_2_1 = 6,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_2_2 = 7,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_3_0 = 8,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_3_1 = 9,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_3_2 = 10,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_4_0 = 11,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_4_1 = 12,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_4_2 = 13,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_5_0 = 14,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_LEVEL_5_1 = 15,
};

enum virtio_video_mpeg_video_hevc_profile {
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_PROFILE_MAIN = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_PROFILE_MAIN_STILL_PICTURE = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_PROFILE_MAIN_10 = 2,
};

enum virtio_video_mpeg_video_hevc_level {
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_1 = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_2 = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_2_1 = 2,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_3 = 3,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_3_1 = 4,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_4 = 5,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_4_1 = 6,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_5 = 7,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_5_1 = 8,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_5_2 = 9,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_6 = 10,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_6_1 = 11,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_LEVEL_6_2 = 12,
};

enum virtio_video_mpeg_video_vp9_profile {
	VIRTIO_VIDEO_MPEG_VIDEO_VP9_PROFILE_0 = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_VP9_PROFILE_1 = 1,
	VIRTIO_VIDEO_MPEG_VIDEO_VP9_PROFILE_2 = 2,
	VIRTIO_VIDEO_MPEG_VIDEO_VP9_PROFILE_3 = 3,
};

enum virtio_video_mpeg_vidc_video_vp9_level {
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_UNUSED = 0,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_1 = 1,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_11 = 2,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_2 = 3,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_21 = 4,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_3 = 5,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_31 = 6,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_4 = 7,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_41 = 8,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_5 = 9,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_51 = 10,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_6 = 11,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_VP9_LEVEL_61 = 12,
};

enum virtio_video_mpeg_vidc_video_mpeg2_profile {
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_MPEG2_PROFILE_SIMPLE = 0,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_MPEG2_PROFILE_MAIN = 1,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_MPEG2_PROFILE_HIGH = 2,
};

enum virtio_video_mpeg_vidc_video_mpeg2_level {
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_MPEG2_LEVEL_0 = 0,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_MPEG2_LEVEL_1 = 1,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_MPEG2_LEVEL_2 = 2,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_MPEG2_LEVEL_3 = 3,
};

enum virtio_video_mpeg_video_hevc_tier {
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_TIER_MAIN = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_HEVC_TIER_HIGH = 1,
};

enum virtio_video_mpeg_video_bitrate_mode {
	VIRTIO_VIDEO_MPEG_VIDEO_BITRATE_MODE_VBR = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_BITRATE_MODE_CBR = 1,
};

enum virtio_video_mpeg_vidc_video_bitrate_mode {
	VIRTIO_VIDEO_MPEG_VIDEO_BITRATE_MODE_CBR_VFR =
	VIRTIO_VIDEO_MPEG_VIDEO_BITRATE_MODE_CBR + 1,
	VIRTIO_VIDEO_MPEG_VIDEO_BITRATE_MODE_MBR,
	VIRTIO_VIDEO_MPEG_VIDEO_BITRATE_MODE_MBR_VFR,
	VIRTIO_VIDEO_MPEG_VIDEO_BITRATE_MODE_CQ,
};

enum virtio_video_mpeg_vidc_video_stream_format {
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_NAL_FORMAT_STARTCODES = 0,
	VIRTIO_VIDEO_MPEG_VIDC_VIDEO_NAL_FORMAT_FOUR_BYTE_LENGTH = 4,
};

enum virtio_video_mpeg_vidc_video_roi_type {
	VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_ROI_TYPE_NONE = 0,
	VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_ROI_TYPE_2BIT = 1,
	VIRTIO_VIDEO_CID_MPEG_VIDC_VIDEO_ROI_TYPE_2BYTE = 2,
};

enum virtio_video_mpeg_video_h264_entropy_mode {
	VIRTIO_VIDEO_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC = 0,
	VIRTIO_VIDEO_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC = 1,
};

/*  Used in the VIDIOC_QUERYCTRL ioctl for querying controls */
struct virtio_video_queryctrl {
	__u32             id;
	__u32             type;    /* enum virtio_video_ctrl_type */
	__u8              name[32];    /* Whatever */
	__s32             minimum;    /* Note signedness */
	__s32             maximum;
	__s32             step;
	__s32             default_value;
	__u32             flags;
	__u32             padding[2];
};

struct virtio_video_querymenu {
	__u32        id;
	__u32        index;
	union {
		__u8    name[32];    /* Whatever */
		__s64    value;
	};
	__u32        padding;
} __attribute__((packed));

struct virtio_video_selection {
	__u32                        type;
	__u32                        target;
	__u32                        flags;
	struct virtio_video_rect     r;
	__u32                        padding[9];
};

struct virtio_video_control {
	__u32             id;
	__s32             value;
};

struct virtio_video_fract {
	__u32   numerator;
	__u32   denominator;
};

struct virtio_video_frmival_stepwise {
	struct virtio_video_fract    min;        /* Minimum frame interval [s] */
	struct virtio_video_fract    max;        /* Maximum frame interval [s] */
	struct virtio_video_fract    step;        /* Frame interval step size [s] */
};

struct virtio_video_frmivalenum {
	__u32            index;        /* Frame format index */
	__u32            pixel_format; /* Pixel format */
	__u32            width;        /* Frame width */
	__u32            height;       /* Frame height */
	__u32            type;         /* Frame interval type the device supports. */
	union {                        /* Frame interval */
		struct virtio_video_fract              discrete;
		struct virtio_video_frmival_stepwise    stepwise;
	};
	__u32            padding[2];  /* Reserved space for future use */
};

/*
 *    C A P T U R E   P A R A M E T E R S
 */
struct virtio_video_captureparm {
	__u32                      capability;    /*  Supported modes */
	__u32                      capturemode;   /*  Current mode */
	struct virtio_video_fract  timeperframe;  /*  Time per frame in seconds */
	__u32                      extendedmode;  /*  Driver-specific extensions */
	__u32                      readbuffers;   /*  # of buffers for read */
	__u32                      padding[4];
};

struct virtio_video_outputparm {
	__u32                      capability;     /*  Supported modes */
	__u32                      outputmode;     /*  Current mode */
	struct virtio_video_fract  timeperframe;   /*  Time per frame in seconds */
	__u32                      extendedmode;   /*  Driver-specific extensions */
	__u32                      writebuffers;   /*  # of buffers for write */
	__u32                      padding[4];
};

/*    Stream type-dependent parameters
 */
struct virtio_video_streamparm {
	__u32     type;            /* enum virtio_video_buf_type */
	union {
		struct virtio_video_captureparm    capture;
		struct virtio_video_outputparm     output;
		__u8    raw_data[200];  /* user-defined */
	} parm;
};

/* The structure must be zeroed before use by the application
   This ensures it can be extended safely in the future. */
struct virtio_video_decoder_cmd {
	__u32 cmd;
	__u32 flags;
	union {
		struct {
		    __u64 pts;
		} stop;
		struct {
		    /* 0 or 1000 specifies normal speed,
		       1 specifies forward single stepping,
		       -1 specifies backward single stepping,
		       >1: playback at speed/1000 of the normal speed,
		       <-1: reverse playback at (-speed/1000) of the normal speed. */
		    __s32 speed;
		    __u32 format;
		} start;
		struct {
		    __u32 data[16];
		} raw;
	};
};

enum virtio_video_memory {
	VIRTIO_VIDEO_MEMORY_MMAP = 1,
	VIRTIO_VIDEO_MEMORY_USERPTR = 2,
	VIRTIO_VIDEO_MEMORY_OVERLAY = 3,
	VIRTIO_VIDEO_MEMORY_DMABUF = 4,
};

struct virtio_video_requestbuffers {
	__u32 count;
	__u32 type;
	__u32 memory;
	__u32 capabilities;
	__u32 padding[1];
};

/*
 *    T I M E C O D E
 */
struct virtio_video_timecode {
	__u32   type;
	__u32   flags;
	__u8    frames;
	__u8    seconds;
	__u8    minutes;
	__u8    hours;
	__u8    userbits[4];
};

struct virtio_video_timeval {
	long long	tv_sec;
	long long	tv_usec;
};

/**
 * struct virtio_video_plane - plane info for multi-planar buffers
 * @bytesused:        number of bytes occupied by data in the plane (payload)
 * @length:        size of this plane (NOT the payload) in bytes
 * @mem_offset:        when memory in the associated struct virtio_video_v4l2_buffer is
 *            VIRTIO_VIDEO_MEMORY_MMAP, equals the offset from the start of
 *            the device memory for this plane (or is a "cookie" that
 *            should be passed to mmap() called on the video node)
 * @userptr:        when memory is VIRTIO_VIDEO_MEMORY_USERPTR, a userspace pointer
 *            pointing to this plane
 * @export_id:            when memory is VIRTIO_VIDEO_MEMORY_DMABUF, a userspace file
 *            descriptor associated with this plane
 * @data_offset:    offset in the plane to the start of data; usually 0,
 *            unless there is a header in front of the data
 *
 * Multi-planar buffers consist of one or more planes, e.g. an YCbCr buffer
 * with two planes can have one plane for Y, and another for interleaved CbCr
 * components. Each plane can reside in a separate memory buffer, or even in
 * a completely separate memory node (e.g. in embedded devices).
 */
struct virtio_video_plane {
	__u32            bytesused;
	__u32            length;
	union {
		__u32            mem_offset;
		unsigned long    userptr;
		__s32            export_id;
	} m;
	__u32            data_offset;
	__u32            padding[11];
};

/**
 * struct virtio_video_v4l2_buffer - video buffer info
 * @index:    id number of the buffer
 * @type:    enum virtio_video_buf_type; buffer type (type == *_MPLANE for
 *        multiplanar buffers);
 * @bytesused:    number of bytes occupied by data in the buffer (payload);
 *        unused (set to 0) for multiplanar buffers
 * @flags:    buffer informational flags
 * @field:    enum virtio_video_field; field order of the image in the buffer
 * @timestamp:    frame timestamp
 * @timecode:    frame timecode
 * @sequence:    sequence count of this frame
 * @memory:    enum virtio_video_memory; the method, in which the actual video data is
 *        passed
 * @offset:    for non-multiplanar buffers with memory == VIRTIO_VIDEO_MEMORY_MMAP;
 *        offset from the start of the device memory for this plane,
 *        (or a "cookie" that should be passed to mmap() as offset)
 * @userptr:    for non-multiplanar buffers with memory == VIRTIO_VIDEO_MEMORY_USERPTR;
 *        a userspace pointer pointing to this buffer
 * @export_id:        for non-multiplanar buffers with memory == VIRTIO_VIDEO_MEMORY_DMABUF;
 *        a userspace file descriptor associated with this buffer
 * @planes:    for multiplanar buffers; userspace pointer to the array of plane
 *        info structs for this buffer
 * @length:    size in bytes of the buffer (NOT its payload) for single-plane
 *        buffers (when type != *_MPLANE); number of elements in the
 *        planes array for multi-plane buffers
 * @input:    input number from which the video data has has been captured
 *
 * Contains data exchanged by application and driver using one of the Streaming
 * I/O methods.
 */
struct virtio_video_v4l2_buffer {
	__u32                           index;
	__u32                           type;
	__u32                           bytesused;
	__u32                           flags;
	__u32                           field;
	struct virtio_video_timeval     timestamp;
	struct virtio_video_timecode    timecode;
	__u32                           sequence;
	/* memory location */
	__u32                           memory;
	union {
		__u32                       offset;
		unsigned long               userptr;
		struct virtio_video_plane* planes;
		__s32                       export_id;
	} m;
	__u32                           length;
	__u32                           padding2;
	__u32                           padding;
};

struct virtio_video_cmd_hdr {
	__le32    type; /* One of enum virtio_video_cmd_type */
	__le32    stream_id;
};

/*  Control flags  */
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_DISABLED            0x0001
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_GRABBED             0x0002
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_READ_ONLY           0x0004
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_UPDATE              0x0008
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_INACTIVE            0x0010
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_SLIDER              0x0020
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_WRITE_ONLY          0x0040
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_VOLATILE            0x0080
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_HAS_PAYLOAD         0x0100
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_EXECUTE_ON_WRITE    0x0200
#define VIRTIO_VIDEO_V4L2_CTRL_FLAG_MODIFY_LAYOUT       0x0400

enum virtio_video_v4l2_ctrl_type {
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_INTEGER = 1,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_BOOLEAN = 2,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_MENU = 3,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_BUTTON = 4,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_INTEGER64 = 5,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_CTRL_CLASS = 6,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_STRING = 7,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_BITMASK = 8,
	VIRTIO_VIDEO_V4L2_CTRL_TYPE_INTEGER_MENU = 9,
};

#define VIRTIO_VIDEO_V4L2_CTRL_MAX_DIMS  (4)
#define VIRTIO_BIT(nr)                   ((1) << (nr))

enum msm_vidc_inst_capability_flags {
	CAP_FLAG_NONE                    = 0,
	CAP_FLAG_DYNAMIC_ALLOWED         = VIRTIO_BIT(0),
	CAP_FLAG_MENU                    = VIRTIO_BIT(1),
	CAP_FLAG_INPUT_PORT              = VIRTIO_BIT(2),
	CAP_FLAG_OUTPUT_PORT             = VIRTIO_BIT(3),
	CAP_FLAG_CLIENT_SET              = VIRTIO_BIT(4),
	CAP_FLAG_BITMASK                 = VIRTIO_BIT(5),
	CAP_FLAG_VOLATILE                = VIRTIO_BIT(6),
	CAP_FLAG_META                    = VIRTIO_BIT(7),
};
/* various Metadata - encoder & decoder */
enum msm_vidc_metadata_bits {
	MSM_VIDC_META_DISABLE          = 0x0,
	MSM_VIDC_META_ENABLE           = 0x1,
	MSM_VIDC_META_TX_INPUT         = 0x2,
	MSM_VIDC_META_TX_OUTPUT        = 0x4,
	MSM_VIDC_META_RX_INPUT         = 0x8,
	MSM_VIDC_META_RX_OUTPUT        = 0x10,
	MSM_VIDC_META_MAX              = 0x20,
};

struct virtio_video_ctrl_config {
	__le32 size;
	__le32 id;
	__u64 name_offset;
	enum virtio_video_v4l2_ctrl_type type;
	__s64 min;
	__s64 max;
	__u64 step;
	__s64 def;
	__u32 dims[VIRTIO_VIDEO_V4L2_CTRL_MAX_DIMS];
	__u32 elem_size;
	__u32 flags;
	__u64 menu_skip_mask;
	__u64 qmenu_offset;
	__u64 qmenu_int_offset;
	unsigned int is_private : 1;
};

enum virtio_video_format {
	/* Raw formats */
	VIRTIO_VIDEO_FORMAT_RAW_MIN = 1,
	VIRTIO_VIDEO_FORMAT_ARGB8888 = VIRTIO_VIDEO_FORMAT_RAW_MIN,
	VIRTIO_VIDEO_FORMAT_BGRA8888,
	VIRTIO_VIDEO_FORMAT_RGBA8888,
	VIRTIO_VIDEO_FORMAT_NV12, /* 12  Y/CbCr 4:2:0  */
	VIRTIO_VIDEO_FORMAT_YUV420, /* 12  YUV 4:2:0     */
	VIRTIO_VIDEO_FORMAT_YVU420, /* 12  YVU 4:2:0     */
	VIRTIO_VIDEO_FORMAT_YUV422, /* 16 YUV 4:2:2 */
	VIRTIO_VIDEO_FORMAT_RAW_MAX = VIRTIO_VIDEO_FORMAT_YUV422,
	/* Coded formats */
	VIRTIO_VIDEO_FORMAT_CODED_MIN = 0x1000,
	VIRTIO_VIDEO_FORMAT_MPEG2 =
	VIRTIO_VIDEO_FORMAT_CODED_MIN, /* MPEG-2 Part 2 */
	VIRTIO_VIDEO_FORMAT_MPEG4, /* MPEG-4 Part 2 */
	VIRTIO_VIDEO_FORMAT_H264, /* H.264 */
	VIRTIO_VIDEO_FORMAT_HEVC, /* HEVC aka H.265*/
	VIRTIO_VIDEO_FORMAT_VP8, /* VP8 */
	VIRTIO_VIDEO_FORMAT_VP9, /* VP9 */
	VIRTIO_VIDEO_FORMAT_CODED_MAX = VIRTIO_VIDEO_FORMAT_VP9,
};

enum virtio_video_profile {
	/* H.264 */
	VIRTIO_VIDEO_PROFILE_H264_MIN = 0x100,
	VIRTIO_VIDEO_PROFILE_H264_BASELINE = VIRTIO_VIDEO_PROFILE_H264_MIN,
	VIRTIO_VIDEO_PROFILE_H264_MAIN,
	VIRTIO_VIDEO_PROFILE_H264_EXTENDED,
	VIRTIO_VIDEO_PROFILE_H264_HIGH,
	VIRTIO_VIDEO_PROFILE_H264_HIGH10PROFILE,
	VIRTIO_VIDEO_PROFILE_H264_HIGH422PROFILE,
	VIRTIO_VIDEO_PROFILE_H264_HIGH444PREDICTIVEPROFILE,
	VIRTIO_VIDEO_PROFILE_H264_SCALABLEBASELINE,
	VIRTIO_VIDEO_PROFILE_H264_SCALABLEHIGH,
	VIRTIO_VIDEO_PROFILE_H264_STEREOHIGH,
	VIRTIO_VIDEO_PROFILE_H264_MULTIVIEWHIGH,
	VIRTIO_VIDEO_PROFILE_H264_MAX = VIRTIO_VIDEO_PROFILE_H264_MULTIVIEWHIGH,
	/* HEVC */
	VIRTIO_VIDEO_PROFILE_HEVC_MIN = 0x200,
	VIRTIO_VIDEO_PROFILE_HEVC_MAIN = VIRTIO_VIDEO_PROFILE_HEVC_MIN,
	VIRTIO_VIDEO_PROFILE_HEVC_MAIN10,
	VIRTIO_VIDEO_PROFILE_HEVC_MAIN_STILL_PICTURE,
	VIRTIO_VIDEO_PROFILE_HEVC_MAX =
	VIRTIO_VIDEO_PROFILE_HEVC_MAIN_STILL_PICTURE,
	/* VP8 */
	VIRTIO_VIDEO_PROFILE_VP8_MIN = 0x300,
	VIRTIO_VIDEO_PROFILE_VP8_PROFILE0 = VIRTIO_VIDEO_PROFILE_VP8_MIN,
	VIRTIO_VIDEO_PROFILE_VP8_PROFILE1,
	VIRTIO_VIDEO_PROFILE_VP8_PROFILE2,
	VIRTIO_VIDEO_PROFILE_VP8_PROFILE3,
	VIRTIO_VIDEO_PROFILE_VP8_MAX = VIRTIO_VIDEO_PROFILE_VP8_PROFILE3,
	/* VP9 */
	VIRTIO_VIDEO_PROFILE_VP9_MIN = 0x400,
	VIRTIO_VIDEO_PROFILE_VP9_PROFILE0 = VIRTIO_VIDEO_PROFILE_VP9_MIN,
	VIRTIO_VIDEO_PROFILE_VP9_PROFILE1,
	VIRTIO_VIDEO_PROFILE_VP9_PROFILE2,
	VIRTIO_VIDEO_PROFILE_VP9_PROFILE3,
	VIRTIO_VIDEO_PROFILE_VP9_MAX = VIRTIO_VIDEO_PROFILE_VP9_PROFILE3,
};

enum virtio_video_level {
	/* H.264 */
	VIRTIO_VIDEO_LEVEL_H264_MIN = 0x100,
	VIRTIO_VIDEO_LEVEL_H264_1_0 = VIRTIO_VIDEO_LEVEL_H264_MIN,
	VIRTIO_VIDEO_LEVEL_H264_1_1,
	VIRTIO_VIDEO_LEVEL_H264_1_2,
	VIRTIO_VIDEO_LEVEL_H264_1_3,
	VIRTIO_VIDEO_LEVEL_H264_2_0,
	VIRTIO_VIDEO_LEVEL_H264_2_1,
	VIRTIO_VIDEO_LEVEL_H264_2_2,
	VIRTIO_VIDEO_LEVEL_H264_3_0,
	VIRTIO_VIDEO_LEVEL_H264_3_1,
	VIRTIO_VIDEO_LEVEL_H264_3_2,
	VIRTIO_VIDEO_LEVEL_H264_4_0,
	VIRTIO_VIDEO_LEVEL_H264_4_1,
	VIRTIO_VIDEO_LEVEL_H264_4_2,
	VIRTIO_VIDEO_LEVEL_H264_5_0,
	VIRTIO_VIDEO_LEVEL_H264_5_1,
	VIRTIO_VIDEO_LEVEL_H264_MAX = VIRTIO_VIDEO_LEVEL_H264_5_1,
};

/*
 * Config
 */
struct virtio_video_config {
	__le32 version;
	__le32 max_caps_length;
	__le32 max_resp_length;
};

/*
 * Commands
 */
enum virtio_video_cmd_type {
	/* Command */
	VIRTIO_VIDEO_CMD_QUERY_CAPABILITY = 0x0100,
	VIRTIO_VIDEO_CMD_STREAM_CREATE,
	VIRTIO_VIDEO_CMD_STREAM_DESTROY,
	VIRTIO_VIDEO_CMD_STREAM_DRAIN,
	VIRTIO_VIDEO_CMD_RESOURCE_ATTACH,
	VIRTIO_VIDEO_CMD_RESOURCE_QUEUE,
	VIRTIO_VIDEO_CMD_QUEUE_DETACH_RESOURCES,
	VIRTIO_VIDEO_CMD_QUEUE_CLEAR,
	VIRTIO_VIDEO_CMD_GET_PARAMS,
	VIRTIO_VIDEO_CMD_SET_PARAMS,
	VIRTIO_VIDEO_CMD_QUERY_CONTROL,
	VIRTIO_VIDEO_CMD_GET_CONTROL,
	VIRTIO_VIDEO_CMD_SET_CONTROL,
	VIRTIO_VIDEO_CMD_STREAMON,
	VIRTIO_VIDEO_CMD_STREAMOFF,
	VIRTIO_VIDEO_CMD_STREAM_START,
	/* Response */
	VIRTIO_VIDEO_RESP_OK_NODATA = 0x0200,
	VIRTIO_VIDEO_RESP_OK_QUERY_CAPABILITY,
	VIRTIO_VIDEO_RESP_OK_RESOURCE_QUEUE,
	VIRTIO_VIDEO_RESP_OK_GET_PARAMS,
	VIRTIO_VIDEO_RESP_OK_QUERY_CONTROL,
	VIRTIO_VIDEO_RESP_OK_GET_CONTROL,
	VIRTIO_VIDEO_RESP_ERR_INVALID_OPERATION = 0x0300,
	VIRTIO_VIDEO_RESP_ERR_OUT_OF_MEMORY,
	VIRTIO_VIDEO_RESP_ERR_INVALID_STREAM_ID,
	VIRTIO_VIDEO_RESP_ERR_INVALID_RESOURCE_ID,
	VIRTIO_VIDEO_RESP_ERR_INVALID_PARAMETER,
	VIRTIO_VIDEO_RESP_ERR_UNSUPPORTED_CONTROL,
};

enum virtio_video_sub_cmd_type
{
	ENUM_FMT = 1,
	ENUM_FRAMESIZES,
	ENUM_FRAMEINTERVALS,
	S_FMT,
	G_FMT,
	QUERYCAP,
	SUBSCRIBE_EVENT,
	UNSUBSCRIBE_EVENT,
	QBUF,
	REQBUFS,
	QUERYCTRL,
	QUERYMENU,
	G_CTRL,
	S_CTRL,
	G_PARAM,
	S_PARAM,
	G_SELECTION,
	S_SELECTION,
};

/* VIRTIO_VIDEO_CMD_QUERY_CAPABILITY */
enum virtio_video_queue_type {
	VIRTIO_VIDEO_QUEUE_TYPE_INPUT = 0x100,
	VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT,
	VIRTIO_VIDEO_QUEUE_TYPE_INPUT_META,
	VIRTIO_VIDEO_QUEUE_TYPE_OUTPUT_META,
};

struct virtio_video_query_capability {
	struct virtio_video_cmd_hdr hdr;
	__le32 queue_type; /* One of VIRTIO_VIDEO_QUEUE_TYPE_* types */
	__le32 device_type;
};

enum virtio_video_planes_layout_flag {
	VIRTIO_VIDEO_PLANES_LAYOUT_SINGLE_BUFFER = 1 << 0,
	VIRTIO_VIDEO_PLANES_LAYOUT_PER_PLANE = 1 << 1,
};

struct virtio_video_format_range {
	__le32 min;
	__le32 max;
	__le32 step;
	__u8 padding[4];
};

struct virtio_video_format_frame {
	struct virtio_video_format_range width;
	struct virtio_video_format_range height;
	__le32 num_rates;
	__u8 padding[4];
	/* Followed by struct virtio_video_format_range frame_rates[] */
};

struct virtio_video_format_desc {
	__le64 mask;
	__le32 format; /* One of VIRTIO_VIDEO_FORMAT_* types */
	__le32 planes_layout; /* Bitmask with VIRTIO_VIDEO_PLANES_LAYOUT_* */
	__le32 plane_align;
	__le32 num_frames;
	/* Followed by struct virtio_video_format_frame frames[] */
};

struct virtio_video_query_capability_resp {
	struct virtio_video_cmd_hdr hdr;
	__le32 num_descs;
	__u8 padding[4];
	/* Followed by struct virtio_video_format_desc descs[] */
};

/* VIRTIO_VIDEO_CMD_STREAM_CREATE */
enum virtio_video_mem_type {
	VIRTIO_VIDEO_MEM_TYPE_GUEST_PAGES,
};

struct virtio_video_stream_ioctl_hdr {
	__le32  cmd_type;
	__le32  stream_id;
	__le32  sub_cmd_type;
};

struct virtio_video_stream_ioctl_cmd {
	struct virtio_video_stream_ioctl_hdr hdr;
	__u8   payload[];
};

struct virtio_video_msg {
	struct virtio_video_cmd_hdr hdr;
	__u8   payload[MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE - sizeof(struct virtio_video_cmd_hdr)];
};

struct virtio_video_resp
{
	__le32 result; /* VIRTIO_VIDEO_RESULT_* */
};

struct virtio_video_stream_create {
	struct virtio_video_cmd_hdr hdr;
	__le32 in_mem_type; /* One of VIRTIO_VIDEO_MEM_TYPE_* types */
	__le32 out_mem_type; /* One of VIRTIO_VIDEO_MEM_TYPE_* types */
	__le32 coded_format; /* One of VIRTIO_VIDEO_FORMAT_* types */
	__le32 device_type;  /* virtio_video_device_type types */
	__u8 tag[64];
};

/* VIRTIO_VIDEO_CMD_STREAM_DESTROY */
struct virtio_video_stream_destroy {
	struct virtio_video_cmd_hdr hdr;
};

/* VIRTIO_VIDEO_CMD_STREAM_CREATE */
struct virtio_video_stream_create_resp
{
	__le32 result; /* VIRTIO_VIDEO_RESULT_* */
	__le32 stream_id;
};

struct virtio_video_stream_destroy_resp
{
	__le32 result; /* VIRTIO_VIDEO_RESULT_* */
};

/* VIRTIO_VIDEO_CMD_STREAM_DRAIN */
struct virtio_video_stream_drain {
	struct virtio_video_cmd_hdr hdr;
};

/* VIRTIO_VIDEO_CMD_RESOURCE_ATTACH */
struct virtio_video_resource_object {
	__u8 uuid[16];
};

struct virtio_video_resource_sg_entry {
	__le64 addr;
	__le32 length;
	__u8 padding[4];
};

struct virtio_video_resource_sg_list {
	__le32 num_entries;
	__u8 padding[4];
	struct virtio_video_resource_sg_entry entries[];
};

#define VIRTIO_VIDEO_RESOURCE_SG_SIZE(n) \
	offsetof(struct virtio_video_resource_sg_list, entries[n])

union virtio_video_resource {
	struct virtio_video_resource_sg_list sg_list;
	struct virtio_video_resource_object object;
};

struct virtio_video_resource_attach {
	__le32 cmd_type;
	__le32 stream_id;
	__le32 queue_type; /* VIRTIO_VIDEO_QUEUE_TYPE_* */
	__le32 resource_id;
	/* Followed by struct virtio_video_resource resources[] */
};

/* VIRTIO_VIDEO_CMD_RESOURCE_QUEUE */
struct virtio_video_resource_queue {
	__le32 cmd_type;
	__le32 stream_id;
	__le32 queue_type; /* VIRTIO_VIDEO_QUEUE_TYPE_* */
	__le32 resource_id;
	__le32 flags;      /* Bitmask with VIRTIO_VIDEO_ENQUEUE_FLAG_ * */
	__u8 padding[4];
	__le64 timestamp;
	__le32 data_sizes[VIRTIO_VIDEO_MAX_PLANES];
};

enum virtio_video_dequeue_flag {
	VIRTIO_VIDEO_DEQUEUE_FLAG_ERR = 0,
	VIRTIO_VIDEO_DEQUEUE_FLAG_EOS,
	/* Encoder only */
	VIRTIO_VIDEO_DEQUEUE_FLAG_KEY_FRAME,
	VIRTIO_VIDEO_DEQUEUE_FLAG_PFRAME,
	VIRTIO_VIDEO_DEQUEUE_FLAG_BFRAME,
};

struct virtio_video_resource_queue_resp {
	struct virtio_video_cmd_hdr hdr;
	__le32 flags;
	__le64 timestamp;
	__le32 data_sizes[VIRTIO_VIDEO_MAX_PLANES];
};

/* VIRTIO_VIDEO_CMD_QUEUE_DETACH_RESOURCES */
struct virtio_video_queue_detach_resources {
	__le32 cmd_type;
	__le32 stream_id;
	__le32 queue_type; /* One of VIRTIO_VIDEO_QUEUE_TYPE_* types */
	__u8 padding[4];
};

/* VIRTIO_VIDEO_CMD_QUEUE_CLEAR */
struct virtio_video_queue_clear {
	struct virtio_video_cmd_hdr hdr;
	__le32 queue_type; /* One of VIRTIO_VIDEO_QUEUE_TYPE_* types */
	__u8 padding[4];
};

/* VIRTIO_VIDEO_CMD_GET_PARAMS */
struct virtio_video_plane_format {
	__le32 plane_size;
	__le32 stride;
};

struct virtio_video_crop {
	__le32 left;
	__le32 top;
	__le32 width;
	__le32 height;
};

struct virtio_video_params {
	__le32 queue_type; /* One of VIRTIO_VIDEO_QUEUE_TYPE_* types */
	__le32 format; /* One of VIRTIO_VIDEO_FORMAT_* types */
	__le32 frame_width;
	__le32 frame_height;
	__le32 min_buffers;
	__le32 max_buffers;
	struct virtio_video_crop crop;
	__le32 frame_rate;
	__le32 num_planes;
	struct virtio_video_plane_format plane_formats[VIRTIO_VIDEO_MAX_PLANES];
};

struct virtio_video_get_params {
	struct virtio_video_cmd_hdr hdr;
	__le32 queue_type; /* One of VIRTIO_VIDEO_QUEUE_TYPE_* types */
	__u8 padding[4];
};

struct virtio_video_get_params_resp {
	struct virtio_video_cmd_hdr hdr;
	struct virtio_video_params params;
};

/* VIRTIO_VIDEO_CMD_SET_PARAMS */
struct virtio_video_set_params {
	struct virtio_video_cmd_hdr hdr;
	struct virtio_video_params params;
};

/* VIRTIO_VIDEO_CMD_QUERY_CONTROL */
enum virtio_video_control_type {
	VIRTIO_VIDEO_CONTROL_BITRATE = 1,
	VIRTIO_VIDEO_CONTROL_PROFILE,
	VIRTIO_VIDEO_CONTROL_LEVEL,
};

struct virtio_video_query_control_profile {
	__le32 format; /* One of VIRTIO_VIDEO_FORMAT_* */
	__u8 padding[4];
};

struct virtio_video_query_control_level {
	__le32 format; /* One of VIRTIO_VIDEO_FORMAT_* */
	__u8 padding[4];
};

struct virtio_video_query_control {
	struct virtio_video_cmd_hdr hdr;
	__le32 control; /* One of VIRTIO_VIDEO_CONTROL_* types */
	__u8 padding[4];
	/*
	 * Followed by a value of struct virtio_video_query_control_*
	 * in accordance with the value of control.
	 */
};

struct virtio_video_query_control_resp_profile {
	__le32 num;
	__u8 padding[4];
	/* Followed by an array le32 profiles[] */
};

struct virtio_video_query_control_resp_level {
	__le32 num;
	__u8 padding[4];
	/* Followed by an array le32 level[] */
};

struct virtio_video_query_control_resp {
	struct virtio_video_cmd_hdr hdr;
	/* Followed by one of struct virtio_video_query_control_resp_* */
};

/* VIRTIO_VIDEO_CMD_GET_CONTROL */
struct virtio_video_get_control {
	struct virtio_video_cmd_hdr hdr;
	__le32 control; /* One of VIRTIO_VIDEO_CONTROL_* types */
	__u8 padding[4];
};

struct virtio_video_control_val_bitrate {
	__le32 bitrate;
	__u8 padding[4];
};

struct virtio_video_control_val_profile {
	__le32 profile;
	__u8 padding[4];
};

struct virtio_video_control_val_level {
	__le32 level;
	__u8 padding[4];
};

struct virtio_video_get_control_resp {
	struct virtio_video_cmd_hdr hdr;
	/* Followed by one of struct virtio_video_control_val_* */
};

/* VIRTIO_VIDEO_CMD_SET_CONTROL */
struct virtio_video_set_control {
	struct virtio_video_cmd_hdr hdr;
	__le32 control; /* One of VIRTIO_VIDEO_CONTROL_* types */
	__u8 padding[4];
	/* Followed by one of struct virtio_video_control_val_* */
};

struct virtio_video_set_control_resp {
	struct virtio_video_cmd_hdr hdr;
};

/*
 * Events
 */
enum virtio_video_event_type {
	/* For all devices */
	VIRTIO_VIDEO_EVENT_ERROR = 0x0100,
	VIRTIO_VIDEO_EVENT_FBD,
	VIRTIO_VIDEO_EVENT_EBD,
	/* For decoder only */
	VIRTIO_VIDEO_EVENT_DECODER_RESOLUTION_CHANGED = 0x0200,
};

struct virtio_video_event {
	__le32 event_type; /* One of VIRTIO_VIDEO_EVENT_* types */
	__le32 stream_id;
	__u8 payload[MAX_VIRTIO_VIDEO_CMD_PAYLOAD_SIZE - 2 * sizeof(__le32)];
};

#endif /* _UAPI_LINUX_VIRTIO_VIDEO_H */
