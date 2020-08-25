load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "function",
    srcs = ["function.c"],
    hdrs = ["function.h"],
    deps = [
        "//entity:object",
        "@memory-wrapper//alloc",
        "@memory-wrapper//debug",
        "@memory-wrapper//struct:map",
    ],
)
