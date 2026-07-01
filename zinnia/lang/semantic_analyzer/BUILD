load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
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
        "//zinnia/alloc",
        "//zinnia/lang/parser:lang_parser",
        "//zinnia/program:tape",
        "//zinnia/util:error",
        "//zinnia/util:void_array",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_language_tools//language-tools/lexer:token",
        "@jeffmanzione_language_tools//language-tools/parser",
        "@jeffmanzione_language_tools//language-tools/semantic_analyzer",
        "@jeffmanzione_language_tools//language-tools/semantic_analyzer:expression_tree",
    ],
)

cc_binary(
    name = "semantic_analyzer_test",
    srcs = ["semantic_analyzer_test.c"],
    data = ["//zinnia/lib:lib_srcs"],
    deps = [
        ":definitions",
        "//zinnia/alloc",
        "//zinnia/lang/lexer:lang_lexer",
        "//zinnia/lang/parser:lang_parser",
        "//zinnia/program:tape",
        "//zinnia/util:error",
        "//zinnia/util:void_array",
        "//zinnia/vm:intern",
        "//zinnia/vm:module_manager",
        "@jeffmanzione_file_utils//file-utils:file_info",
        "@jeffmanzione_file_utils//file-utils:sfile",
        "@jeffmanzione_language_tools//language-tools/lexer:token",
        "@jeffmanzione_language_tools//language-tools/parser",
    ],
)
