load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:private"],
)

package_group(
    name = "internal",
    packages = ["//zinnia/..."],
)

cc_library(
    name = "native",
    hdrs = ["native.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//zinnia/value/class:classes_def",
        "//zinnia/value/native/builder",
        "//zinnia/value/native/builder:function_context",
        "//zinnia/value/tuple",
        "//zinnia/util:platform",
    ],
)

cc_binary(
    name = "zinnia",
    srcs = ["zinnia.c"],
    defines = [
        "NDEBUG",
    ],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/run"],
)

cc_binary(
    name = "zinniac",
    srcs = ["zinniac.c"],
    defines = [
        "NDEBUG",
    ],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/compile"],
)

cc_binary(
    name = "zinnia-package",
    srcs = ["zinnia_package.c"],
    defines = [
        "NDEBUG",
    ],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/package"],
)

cc_binary(
    name = "zinnia-bundle",
    srcs = ["zinnia_bundle.c"],
    defines = [
        "NDEBUG",
    ],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/bundle"],
)
