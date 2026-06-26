load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "function",
    srcs = ["function.c"],
    hdrs = ["function.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/heap",
        "//zinnia/util:error",
    ],
)
