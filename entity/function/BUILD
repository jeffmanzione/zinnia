load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "function",
    srcs = ["function.c"],
    hdrs = ["function.h"],
    deps = [
        "//entity",
        "//entity:object",
        "//heap",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
    ],
)
