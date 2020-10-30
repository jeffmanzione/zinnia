load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "string",
    srcs = ["string.c"],
    hdrs = ["string.h"],
    deps = [],
)

cc_library(
    name = "file",
    srcs = ["file.c"],
    hdrs = ["file.h"],
    deps = [
        ":string",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)
