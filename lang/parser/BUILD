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
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:struct_defaults",
    ],
)
