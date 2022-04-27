load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_benchmark_binary",
    "envoy_contrib_package",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

envoy_cc_benchmark_binary(
    name = "dml_speed_test",
    srcs = ["dml_speed_test.cc"],
    external_deps = ["benchmark"],
    deps = [
        "//contrib/dml/source:dml",
        "//source/common/common:assert_lib",
    ],
)
