load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "module",
    srcs = ["module.c"],
    hdrs = ["module.h"],
    deps = [
        "//entity:object",
        "//entity/class",
        "//entity/function",
        "//program:tape",
        "//struct:keyed_list",
    ],
)
