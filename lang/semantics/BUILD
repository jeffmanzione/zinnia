load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "expression_macro",
    hdrs = ["expression_macros.h"],
    deps = [
        "//lang/lexer:token",
        "//lang/parser",
        "//struct:alist",
        "@memory-wrapper//struct:map",
    ],
)

cc_library(
    name = "expression_tree",
    hdrs = ["expression_tree.h"],
)

cc_library(
    name = "semantics",
    srcs = ["semantics.c"],
    hdrs = ["semantics.h"],
    deps = [
        ":expression_macro",
        ":expression_tree",
        "//lang/parser",
        "//program:tape",
    ],
)
