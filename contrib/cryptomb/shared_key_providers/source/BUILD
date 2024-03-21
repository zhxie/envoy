load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_contrib_extension",
    "envoy_cc_library",
    "envoy_contrib_package",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

envoy_cc_library(
    name = "cryptomb_shared_key_provider_lib",
    srcs = [
        "cryptomb_shared_key_provider.cc",
    ],
    hdrs = [
        "cryptomb_shared_key_provider.h",
    ],
    external_deps = ["ssl"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":cryptomb_stats_lib",
        "//contrib/cryptomb/private_key_providers/source:ipp_crypto_wrapper_lib",
        "//envoy/api:api_interface",
        "//envoy/event:dispatcher_interface",
        "//envoy/registry",
        "//envoy/server:transport_socket_config_interface",
        "//envoy/singleton:manager_interface",
        "//envoy/ssl/shared_key:shared_key_config_interface",
        "//envoy/ssl/shared_key:shared_key_interface",
        "//source/common/common:logger_lib",
        "//source/common/common:thread_lib",
        "//source/common/config:datasource_lib",
        "@envoy_api//contrib/envoy/extensions/shared_key_providers/cryptomb/v3alpha:pkg_cc_proto",
    ],
)

envoy_cc_library(
    name = "cryptomb_stats_lib",
    srcs = [
        "cryptomb_stats.cc",
    ],
    hdrs = [
        "cryptomb_stats.h",
    ],
    deps = [
        "//envoy/stats:stats_interface",
        "//envoy/stats:stats_macros",
        "//source/common/stats:symbol_table_lib",
        "//source/common/stats:utility_lib",
    ],
)

envoy_cc_contrib_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    defines = select({
        "//bazel:linux_x86_64": [],
        "//conditions:default": [
            "IPP_CRYPTO_DISABLED=1",
        ],
    }),
    deps = [
        "//envoy/registry",
        "//envoy/ssl/shared_key:shared_key_config_interface",
        "//envoy/ssl/shared_key:shared_key_interface",
        "//source/common/common:logger_lib",
        "//source/common/config:utility_lib",
        "//source/common/protobuf:utility_lib",
        "@envoy_api//contrib/envoy/extensions/shared_key_providers/cryptomb/v3alpha:pkg_cc_proto",
        "@envoy_api//envoy/extensions/transport_sockets/tls/v3:pkg_cc_proto",
    ] + select({
        "//bazel:linux_x86_64": [
            ":cryptomb_shared_key_provider_lib",
            "//contrib/cryptomb/private_key_providers/source:ipp_crypto_wrapper_lib",
        ],
        "//conditions:default": [
        ],
    }),
)
