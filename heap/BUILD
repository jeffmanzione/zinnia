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
        "//entity/array",
        "//entity/string",
        "//entity/tuple",
        "//struct:alist",
        "//struct:keyed_list",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena",
        "@memory_wrapper//alloc/memory_graph",
    ],
)
