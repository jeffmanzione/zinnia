load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "instruction",
    srcs = ["instruction.c"],
    hdrs = ["instruction.h"],
    deps = [
        ":op",
        "//entity:primitive",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "op",
    srcs = ["op.c"],
    hdrs = ["op.h"],
    deps = [],
)

cc_library(
    name = "tape",
    srcs = ["tape.c"],
    hdrs = ["tape.h"],
    deps = [
        ":instruction",
        "//lang/lexer:token",
        "//struct:alist",
        "//struct:keyed_list",
        "//struct:q",
        "@memory-wrapper//alloc",
        "@memory-wrapper//alloc/arena:intern",
        "@memory-wrapper//debug",
    ],
)
