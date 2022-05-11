load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_contrib_extension",
    "envoy_cc_library",
    "envoy_contrib_package",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

envoy_cc_library(
    name = "idxd",
    external_deps = ["idxd-config"],
)

envoy_cc_contrib_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    defines = select({
        "//bazel:linux_x86_64": [],
        "//conditions:default": [
            "IDXD_DISABLED=1",
        ],
    }),
    deps = [
        "//source/common/buffer:memory_interface_lib",
        "@envoy_api//contrib/envoy/extensions/idxd/v3alpha:pkg_cc_proto",
    ] + select({
        "//bazel:linux_x86_64": [
            ":idxd",
        ],
        "//conditions:default": [
        ],
    }),
)
