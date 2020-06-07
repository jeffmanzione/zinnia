load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//lang/semantics:__pkg__"],
)

cc_library(
    name = "assignment",
    srcs = ["assignment.c"],
    hdrs = ["assignment.h"],
    deps = [
        "//lang/lexer:token",
        "//lang/parser",
        "//lang/semantics:expression_macros",
        "//program:tape",
        "//struct:alist",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "classes",
    srcs = ["classes.c"],
    hdrs = ["classes.h"],
    deps = [
        ":files",
        "//lang/lexer:token",
        "//lang/parser",
        "//lang/semantics:expression_macros",
        "//program:tape",
        "//struct:alist",
        "//struct:q",
        "//vm:intern",
        "@memory-wrapper//alloc/arena:intern",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "expression",
    srcs = ["expression.c"],
    hdrs = ["expression.h"],
    deps = [
        ":files",
        "//lang/lexer:token",
        "//lang/parser",
        "//lang/semantics:expression_macros",
        "//program:tape",
        "//struct:alist",
        "//struct:q",
        "@memory-wrapper//alloc",
        "@memory-wrapper//alloc/arena:intern",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "files",
    srcs = ["files.c"],
    hdrs = ["files.h"],
    deps = [
        ":assignment",
        "//lang/lexer:token",
        "//lang/parser",
        "//lang/semantics:expression_macros",
        "//program:tape",
        "//struct:alist",
        "//vm:intern",
        "@memory-wrapper//alloc/arena:intern",
        "@memory-wrapper//debug",
    ],
)

cc_library(
    name = "loops",
    srcs = ["loops.c"],
    hdrs = ["loops.h"],
    deps = [
        "//vm:intern",
    ],
)

cc_library(
    name = "statements",
    srcs = ["statements.c"],
    hdrs = ["statements.h"],
    deps = [
        "//vm:intern",
    ],
)
