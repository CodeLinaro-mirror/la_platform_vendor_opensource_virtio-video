load(":virtio_video_driver_build.bzl", "video_module_entry")

video_driver_modules = video_module_entry([":virtio_video_driver_headers"])
module_entry = video_driver_modules.register

module_entry(
    name = "msm_virtio_video",
    srcs = [
        "driver/virtio_video_vq.c",
        "driver/virtio_video_msm_mem.c",
        "driver/virtio_video_msm_hab.c",
        "driver/virtio_video_dec.c",
        "driver/virtio_video_msm_debug.c",
        "driver/virtio_video_driver.c",
        "driver/virtio_video_msm_vq.c",
        "driver/virtio_video_msm_v4l2.c",
        "driver/virtio_video_caps.c",
        "driver/virtio_video_cam.c",
        "driver/virtio_video_msm_vb2.c",
        "driver/virtio_video_helpers.c",
        "driver/virtio_video_enc.c",
        "driver/virtio_video_msm_hab.c",
        "driver/virtio_video_device.c",
        "driver/virtio_video_msm_hw_virt.c",
        ],
)
