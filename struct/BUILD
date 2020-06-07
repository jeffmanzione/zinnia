load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "alist",
    srcs = ["alist.c"],
    hdrs = ["alist.h"],
    deps = [
        "@memory-wrapper//alloc",
        "@memory-wrapper//debug",
        "@memory-wrapper//util",
    ],
)

cc_library(
    name = "keyed_list",
    srcs = ["keyed_list.c"],
    hdrs = ["keyed_list.h"],
    deps = [
        ":alist",
        "@memory-wrapper//alloc",
        "@memory-wrapper//debug",
        "@memory-wrapper//struct:map",
        "@memory-wrapper//struct:struct_defaults",
    ],
)

cc_library(
    name = "arraylike",
    hdrs = ["arraylike.h"],
    deps = [
        "@memory-wrapper//alloc",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "q",
    srcs = ["q.c"],
    hdrs = ["q.h"],
    deps = [":arraylike"],
)
