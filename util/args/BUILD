load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "commandline",
    srcs = ["commandline.c"],
    hdrs = ["commandline.h"],
    deps = [
        ":commandline_arg",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:set",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_library(
    name = "commandlines",
    srcs = ["commandlines.c"],
    hdrs = ["commandlines.h"],
    deps = [
        ":commandline",
        ":commandline_arg",
        ":lib_finder",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "commandline_arg",
    srcs = ["commandline_arg.c"],
    hdrs = ["commandline_arg.h"],
    deps = [
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "lib_finder",
    srcs = ["lib_finder.c"],
    hdrs = ["lib_finder.h"],
    deps = [
        "@memory_wrapper//alloc/arena:intern",
    ],
)
