load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "shared_key_manager_lib",
    srcs = [
        "shared_key_manager_impl.cc",
    ],
    hdrs = [
        "shared_key_manager_impl.h",
    ],
    deps = [
        "//envoy/event:dispatcher_interface",
        "//envoy/registry",
        "//envoy/ssl/shared_key:shared_key_config_interface",
        "//envoy/ssl/shared_key:shared_key_interface",
        "@envoy_api//envoy/extensions/transport_sockets/tls/v3:pkg_cc_proto",
    ],
)
