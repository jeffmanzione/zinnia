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
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
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
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "token",
    srcs = ["token.c"],
    hdrs = ["token.h"],
    deps = [
        "//entity:primitive",
        "@memory_wrapper//alloc/arena",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)
