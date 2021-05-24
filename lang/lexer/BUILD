load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "lexer",
    srcs = ["lexer.c"],
    hdrs = ["lexer.h"],
    deps = [
        ":token",
        "@c_data_structures//struct:q",
        "@file_utils//util:string",
        "@file_utils//util/file:file_info",
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
