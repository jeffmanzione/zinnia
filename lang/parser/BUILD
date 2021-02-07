load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "parser",
    srcs = ["parser.c"],
    hdrs = ["parser.h"],
    deps = [
        "//lang/lexer",
        "//lang/lexer:token",
        "//struct:q",
        "@file_utils//util/file:file_info",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:struct_defaults",
    ],
)
