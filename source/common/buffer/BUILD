load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "watermark_buffer_lib",
    srcs = ["watermark_buffer.cc"],
    hdrs = ["watermark_buffer.h"],
    deps = [
        "//envoy/http:stream_reset_handler_interface",
        "//source/common/buffer:buffer_lib",
        "//source/common/common:assert_lib",
        "//source/common/runtime:runtime_features_lib",
        "@envoy_api//envoy/config/overload/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "buffer_lib",
    srcs = ["buffer_impl.cc"],
    hdrs = ["buffer_impl.h"],
    deps = [
        ":memory_interface_lib",
        "//envoy/buffer:buffer_interface",
        "//source/common/common:non_copyable",
        "//source/common/common:thread_lib",
        "//source/common/common:utility_lib",
        "//source/common/event:libevent_lib",
        "//source/common/singleton:threadsafe_singleton",
    ],
)

envoy_cc_library(
    name = "zero_copy_input_stream_lib",
    srcs = ["zero_copy_input_stream_impl.cc"],
    hdrs = ["zero_copy_input_stream_impl.h"],
    deps = [
        ":buffer_lib",
        "//source/common/protobuf",
    ],
)

envoy_cc_library(
    name = "memory_interface_lib",
    hdrs = ["memory_interface.h"],
    deps = [
        "//envoy/buffer:memory_interface",
        "//envoy/registry",
        "//envoy/server:bootstrap_extension_config_interface",
    ],
)

envoy_cc_library(
    name = "default_memory_interface_lib",
    srcs = ["memory_interface_impl.cc"],
    hdrs = ["memory_interface_impl.h"],
    deps = [
        ":memory_interface_lib",
        "@envoy_api//envoy/extensions/buffer/memory_interface/v3:pkg_cc_proto",
    ],
)
