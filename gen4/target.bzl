load(":virtio_video_modules.bzl", "video_driver_modules")
load(":virtio_video_driver_build.bzl", "define_lunch_target_variant_modules")
load(":target_variants.bzl", "get_all_variants")

def define_target_modules():
    for (target, variant) in get_all_variants():
        define_lunch_target_variant_modules(
            target = target,
            variant = variant,
            registry = video_driver_modules,
            modules = [
                "msm_virtio_video",
            ],
        )
