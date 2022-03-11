load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "definitions",
    srcs = [
        "definitions.c",
        "files.c",
        "loops.c",
        "statements.c",
    ],
    hdrs = ["definitions.h"],
    deps = [
        "//lang/parser:lang_parser",
        "//program:tape",
        "//vm:intern",
        "@c_data_structures//struct:alist",
        "@c_data_structures//struct:q",
        "@language_tools//lang/lexer:token",
        "@language_tools//lang/parser",
        "@language_tools//lang/semantic_analyzer",
        "@language_tools//lang/semantic_analyzer:expression_tree",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)

cc_binary(
    name = "semantic_analyzer_test",
    srcs = ["semantic_analyzer_test.c"],
    data = ["//lib:lib_srcs"],
    deps = [
        ":definitions",
        "//lang/lexer:lang_lexer",
        "//lang/parser:lang_parser",
        "//program:tape",
        "@c_data_structures//struct:q",
        "@file_utils//util/file:file_info",
        "@file_utils//util/file:sfile",
        "@language_tools//lang/lexer:token",
        "@language_tools//lang/parser",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)
