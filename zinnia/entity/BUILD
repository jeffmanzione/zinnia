load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "entity",
    srcs = ["entity.c"],
    hdrs = ["entity.h"],
    deps = [
        ":object",
        ":primitive",
        "//zinnia/util:error",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
    ],
)

cc_library(
    name = "object",
    srcs = ["object.c"],
    hdrs = ["object.h"],
    deps = [
        "//zinnia/program:tape",
        "//zinnia/util:dll",
        "//zinnia/util/sync:mutex",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
        "@jeffmanzione_memory_wrapper//memory-wrapper:memory_graph",
    ],
)

cc_library(
    name = "primitive",
    srcs = ["primitive.c"],
    hdrs = ["primitive.h"],
)
