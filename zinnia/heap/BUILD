load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "heap",
    srcs = ["heap.c"],
    hdrs = ["heap.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/value:value",
        "//zinnia/value:object",
        "//zinnia/value/array",
        "//zinnia/value/class:classes_def",
        "//zinnia/value/string",
        "//zinnia/value/tuple",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
        "@jeffmanzione_rzalloc//rzalloc",
    ],
)

cc_library(
    name = "copy_fns",
    srcs = ["copy_fns.c"],
    hdrs = ["copy_fns.h"],
    deps = [
        ":heap",
        "//zinnia/value:value",
        "//zinnia/value:object",
        "//zinnia/value/array",
        "//zinnia/value/string",
        "//zinnia/value/tuple",
    ],
)
