load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "function",
    srcs = ["function.c"],
    hdrs = ["function.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/value:value",
        "//zinnia/value:object",
        "//zinnia/heap",
        "//zinnia/util:error",
    ],
)
