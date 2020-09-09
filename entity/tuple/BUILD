load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "tuple",
    srcs = ["tuple.c"],
    hdrs = ["tuple.h"],
    deps = [
        "//entity",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)
