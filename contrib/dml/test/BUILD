load(
    "//bazel:envoy_build_system.bzl",
    "envoy_benchmark_test",
    "envoy_cc_benchmark_binary",
    "envoy_contrib_package",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

envoy_cc_benchmark_binary(
    name = "dml_speed_test",
    srcs = ["dml_speed_test.cc"],
    external_deps = [
        "benchmark",
        "dml",
    ],
)

envoy_benchmark_test(
    name = "dml_speed_test_benchmark_test",
    benchmark_binary = "dml_speed_test",
)
