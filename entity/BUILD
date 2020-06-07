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
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "object",
    srcs = ["object.c"],
    hdrs = ["object.h"],
    deps = [
        "//struct:keyed_list",
        "@memory-wrapper//alloc/memory_graph",
        "@memory-wrapper//struct:map",
    ],
)

cc_library(
    name = "primitive",
    srcs = ["primitive.c"],
    hdrs = ["primitive.h"],
    deps = [
        "@memory-wrapper//debug",
    ],
)
