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
        "//entity/class:classes_def",
        "//entity/string",
        "//entity/tuple",
        "@c_data_structures//struct:alist",
        "@c_data_structures//struct:keyed_list",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena",
        "@memory_wrapper//alloc/memory_graph",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_library(
    name = "copy_fns",
    srcs = ["copy_fns.c"],
    hdrs = ["copy_fns.h"],
    deps = [
        ":heap",
        "//entity",
        "//entity:object",
        "//entity/array",
        "//entity/string",
        "//entity/tuple",
        "@memory_wrapper//struct:map",
    ],
)
