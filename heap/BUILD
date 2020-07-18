load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "heap",
    srcs = ["heap.c"],
    hdrs = ["heap.h"],
    deps = [
        "//entity",
        "//entity:object",
        "//struct:alist",
        "//struct:keyed_list",
        "@memory-wrapper//alloc",
        "@memory-wrapper//alloc/arena",
        "@memory-wrapper//alloc/memory_graph",
    ],
)
