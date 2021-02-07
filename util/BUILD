load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "platform",
    hdrs = ["platform.h"],
    deps = [],
)

cc_library(
    name = "file",
    srcs = ["file.c"],
    hdrs = ["file.h"],
    deps = [
        "@file_utils//util:string",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
    ],
)

cc_library(
    name = "string_util",
    srcs = ["string_util.c"],
    hdrs = ["string_util.h"],
    deps = [
        "@memory_wrapper//alloc",
    ],
)
