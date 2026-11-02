load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "class",
    srcs = ["class.c"],
    hdrs = ["class.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/value:object",
        "//zinnia/value/function",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
    ],
)

cc_library(
    name = "classes_def",
    hdrs = [
        "class.h",
        "classes_def.h",
    ],
    deps = [
        "//zinnia/value:object",
    ],
)

cc_library(
    name = "classes",
    srcs = ["classes.c"],
    hdrs = [
        "classes.h",
        "classes_def.h",
    ],
    deps = [
        ":class",
        ":classes_def",
        "//zinnia/value:object",
        "//zinnia/value/array",
        "//zinnia/value/function",
        "//zinnia/value/module",
        "//zinnia/value/string",
        "//zinnia/value/tuple",
        "//zinnia/heap",
        "//zinnia/heap:copy_fns",
        "//zinnia/util:error",
        "//zinnia/vm:intern",
    ],
)
