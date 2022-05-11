load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_test",
    "envoy_contrib_package",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

envoy_cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    deps = [
        "//contrib/idxd/source:config",
        "//test/mocks/server:factory_context_mocks",
        "@envoy_api//contrib/envoy/extensions/idxd/v3alpha:pkg_cc_proto",
    ],
)
