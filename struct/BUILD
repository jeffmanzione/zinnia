load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "alist",
    srcs = ["alist.c"],
    hdrs = ["alist.h"],
    deps = [
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//util",
    ],
)

cc_library(
    name = "keyed_list",
    srcs = ["keyed_list.c"],
    hdrs = ["keyed_list.h"],
    deps = [
        ":alist",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_library(
    name = "arraylike",
    hdrs = ["arraylike.h"],
    deps = [
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "q",
    srcs = ["q.c"],
    hdrs = ["q.h"],
    deps = [":arraylike"],
)
