load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "tuple",
    srcs = ["tuple.c"],
    hdrs = ["tuple.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/entity",
        "//zinnia/util:error",
    ],
)
