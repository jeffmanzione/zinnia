load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "string",
    srcs = ["string.c"],
    hdrs = ["string.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/value:value",
        "//zinnia/value:object",
        "//zinnia/util:error",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
    ],
)

cc_library(
    name = "string_helper",
    srcs = ["string_helper.c"],
    hdrs = ["string_helper.h"],
    deps = [
        ":string",
        "//zinnia/alloc",
        "//zinnia/value:object",
        "//zinnia/value/class:classes",
        "//zinnia/heap",
        "//zinnia/util:error",
        "//zinnia/vm:intern",
    ],
)
