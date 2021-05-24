load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "entity",
    srcs = ["entity.c"],
    hdrs = ["entity.h"],
    deps = [
        ":object",
        ":primitive",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "object",
    srcs = ["object.c"],
    hdrs = ["object.h"],
    deps = [
        "//lang/lexer:token",
        "//program:tape",
        "//util/sync:mutex",
        "@c_data_structures//struct:alist",
        "@c_data_structures//struct:keyed_list",
        "@memory_wrapper//alloc/memory_graph",
        "@memory_wrapper//struct:map",
    ],
)

cc_library(
    name = "primitive",
    srcs = ["primitive.c"],
    hdrs = ["primitive.h"],
    deps = [
        "@memory_wrapper//debug",
    ],
)
