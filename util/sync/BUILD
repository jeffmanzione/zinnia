load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "mutex",
    srcs = ["mutex.c"],
    hdrs = ["mutex.h"],
    deps = [
        "//util:platform",
        "@memory_wrapper//alloc",
    ],
)
