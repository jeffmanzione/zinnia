load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "class",
    srcs = ["class.c"],
    hdrs = ["class.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/entity:object",
        "//zinnia/entity/function",
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
        "//zinnia/entity:object",
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
        "//zinnia/entity:object",
        "//zinnia/entity/array",
        "//zinnia/entity/function",
        "//zinnia/entity/module",
        "//zinnia/entity/string",
        "//zinnia/entity/tuple",
        "//zinnia/heap",
        "//zinnia/heap:copy_fns",
        "//zinnia/util:error",
        "//zinnia/vm:intern",
    ],
)
