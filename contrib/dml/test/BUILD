load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_benchmark_binary",
    "envoy_cc_test",
    "envoy_cc_test_library",
    "envoy_contrib_package",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

envoy_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    deps = [
        "//contrib/dml/source:config",
        "//test/mocks/server:factory_context_mocks",
        "@envoy_api//contrib/envoy/extensions/dml/v3alpha:pkg_cc_proto",
    ],
)

envoy_cc_test_library(
    name = "bandwidth_lib",
    hdrs = ["bandwidth.h"],
    external_deps = ["benchmark"],
)

envoy_cc_benchmark_binary(
    name = "dml_speed_test",
    srcs = ["dml_speed_test.cc"],
    deps = [
        ":bandwidth_lib",
        "//contrib/dml/source:dml",
        "//source/common/common:assert_lib",
    ],
)

envoy_cc_benchmark_binary(
    name = "memory_interface_speed_test",
    srcs = ["memory_interface_speed_test.cc"],
    deps = [
        ":bandwidth_lib",
        "//contrib/dml/source:config",
        "//source/common/buffer:default_memory_interface_lib",
    ],
)
