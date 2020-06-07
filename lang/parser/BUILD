load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "parser",
    srcs = ["parser.c"],
    hdrs = ["parser.h"],
    copts = ["-std=gnu99"],  # For nested anonymous functions.
    deps = [
        "//lang/lexer",
        "//lang/lexer:file_info",
        "//lang/lexer:token",
        "//struct:q",
        "@memory-wrapper//alloc",
        "@memory-wrapper//debug",
        "@memory-wrapper//struct:map",
        "@memory-wrapper//struct:struct_defaults",
    ],
)
