load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "compile",
    srcs = ["compile.c"],
    hdrs = ["compile.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/lang/parser:lang_parser",
        "//zinnia/program:tape",
        "//zinnia/program:tape_binary",
        "//zinnia/program/optimization:optimize",
        "//zinnia/util:error",
        "//zinnia/util:void_array",
        "//zinnia/util/args:commandline",
        "//zinnia/util/args:commandlines",
        "//zinnia/util/args:lib_finder",
        "//zinnia/vm:module_manager",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
        "@jeffmanzione_file_utils//file-utils:file_info",
        "@jeffmanzione_file_utils//file-utils:file_utils",
        "@jeffmanzione_file_utils//file-utils:string_utils",
        "@jeffmanzione_language_tools//language-tools/lexer:token",
        "@jeffmanzione_language_tools//language-tools/parser",
        "@jeffmanzione_language_tools//language-tools/semantic_analyzer",
        "@jeffmanzione_language_tools//language-tools/semantic_analyzer:expression_tree",
    ],
)
