load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "value",
    srcs = ["value.c"],
    hdrs = ["value.h"],
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
        "//zinnia/program:bytecode",
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
    deps = ["//zinnia/util:error"],
)
