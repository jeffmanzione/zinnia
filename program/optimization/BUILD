load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "optimize",
    srcs = ["optimize.c"],
    hdrs = ["optimize.h"],
    deps = [
        ":optimizer",
        ":optimizers",
        "//entity",
        "//entity:primitive",
        "//lang/lexer:token",
        "//program:instruction",
        "//program:op",
        "@c_data_structures//struct:alist",
        "@c_data_structures//struct:q",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:set",
    ],
)

cc_library(
    name = "optimizer",
    srcs = ["optimizer.c"],
    hdrs = ["optimizer.h"],
    deps = [
        "//program:instruction",
        "//program:tape",
        "@c_data_structures//struct:alist",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
    ],
)

cc_library(
    name = "optimizers",
    srcs = ["optimizers.c"],
    hdrs = ["optimizers.h"],
    deps = [
        ":optimizer",
        "//entity",
        "//program:instruction",
        "//program:tape",
        "//vm:intern",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//struct:map",
    ],
)
