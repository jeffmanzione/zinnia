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
        "@memory_wrapper//debug",
    ],
)
