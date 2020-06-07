load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "class",
    srcs = ["class.c"],
    hdrs = ["class.h"],
    deps = [
        "//entity:object",
        "//entity/function",
        "//entity/module",
        "//struct:keyed_list",
        "@memory-wrapper//alloc",
    ],
)
