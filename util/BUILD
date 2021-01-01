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
    hdrs = ["file.h"],
    deps = [
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
    ],
)
