load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "error",
    srcs = ["error.c"],
    hdrs = ["error.h"],
    deps = [
        "//entity:object",
        "//program:instruction",
        "//struct:alist",
        "@memory_wrapper//debug",
    ],
)
