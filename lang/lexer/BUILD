load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "file_info",
    srcs = ["file_info.c"],
    hdrs = ["file_info.h"],
    deps = [
        ":token",
        "//util:file",
        "@memory-wrapper//alloc",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "lexer",
    srcs = ["lexer.c"],
    hdrs = ["lexer.h"],
    deps = [
        ":file_info",
        ":token",
        "//struct:q",
        "//util:string",
        "@memory-wrapper//alloc",
        "@memory-wrapper//alloc/arena",
        "@memory-wrapper//alloc/arena:intern",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "token",
    srcs = ["token.c"],
    hdrs = ["token.h"],
    deps = [
        "@memory-wrapper//alloc/arena",
        "@memory-wrapper//alloc/arena:intern",
        "@memory-wrapper//debug",
    ],
)
