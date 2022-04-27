load("@bazel_skylib//lib:selects.bzl", "selects")
load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_contrib_extension",
    "envoy_cmake",
    "envoy_contrib_package",
)
load(
    "//contrib:all_contrib_extensions.bzl",
    "envoy_contrib_linux_x86_64_constraints",
)

licenses(["notice"])  # Apache 2

envoy_contrib_package()

envoy_cmake(
    name = "dml",
    cache_entries = {
        "CMAKE_CXX_FLAGS": "-Dlinux",
        "CMAKE_INSTALL_LIBDIR": "lib",
    },
    lib_source = "@com_github_intel_dml//:all",
    out_static_libs = [
        "libdml.a",
        "libdmlhl.a",
    ],
    tags = ["skip_on_windows"],
    target_compatible_with = envoy_contrib_linux_x86_64_constraints(),
    visibility = ["//visibility:private"],
)

selects.config_setting_group(
    name = "linux_x86_64_gcc_build",
    match_all = [
        "//bazel:linux_x86_64",
        "//bazel:gcc_build",
    ],
)

envoy_cc_contrib_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    defines = select({
        ":linux_x86_64_gcc_build": [],
        "//conditions:default": [
            "DML_DISABLED=1",
        ],
    }),
    deps = [
        "//source/common/buffer:memory_interface_lib",
        "@envoy_api//contrib/envoy/extensions/dml/v3alpha:pkg_cc_proto",
    ] + select({
        ":linux_x86_64_gcc_build": [
            ":dml",
        ],
        "//conditions:default": [
        ],
    }),
)
