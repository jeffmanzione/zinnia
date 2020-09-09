load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "string",
    srcs = ["string.c"],
    hdrs = ["string.h"],
    deps = [
        "//entity",
        "//entity:object",
        "//struct:arraylike",
        "@memory_wrapper//debug",
    ],
)
