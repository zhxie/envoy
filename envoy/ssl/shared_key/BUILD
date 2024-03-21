load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "shared_key_interface",
    hdrs = ["shared_key.h"],
    external_deps = ["ssl"],
    deps = [
        ":shared_key_callbacks_interface",
        "//envoy/event:dispatcher_interface",
        "@envoy_api//envoy/extensions/transport_sockets/tls/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "shared_key_config_interface",
    hdrs = ["shared_key_config.h"],
    deps = [
        ":shared_key_interface",
        "//envoy/config:typed_config_interface",
        "//envoy/registry",
        "@envoy_api//envoy/extensions/transport_sockets/tls/v3:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "shared_key_callbacks_interface",
    hdrs = ["shared_key_callbacks.h"],
    external_deps = ["ssl"],
)
