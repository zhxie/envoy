load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_benchmark_binary",
    "envoy_cc_test_library",
    "envoy_contrib_package",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

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
