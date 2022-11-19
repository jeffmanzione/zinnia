load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "named_list",
    srcs = ["named_list.c"],
    hdrs = ["named_list.h"],
    deps = [
        "//entity",
        "@c_data_structures//struct:keyed_list",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)
