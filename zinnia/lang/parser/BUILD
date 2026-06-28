load("@jeffmanzione_language_tools//language-tools/parser:parser_builder.bzl", "parser_builder")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(
    default_visibility = ["//zinnia:internal"],
)

parser_builder(
    name = "lang_parser",
    lexer = "//zinnia/lang/lexer:lang_lexer",
    rules = "rules.txt",
)

cc_binary(
    name = "parser_test",
    srcs = ["parser_test.c"],
    data = ["//zinnia/lib:lib_srcs"],
    deps = [
        ":lang_parser",
        "//zinnia/alloc",
        "//zinnia/lang/lexer:lang_lexer",
        "//zinnia/util:void_array",
        "@jeffmanzione_file_utils//file-utils:file_info",
        "@jeffmanzione_file_utils//file-utils:sfile",
        "@jeffmanzione_language_tools//language-tools:intern",
        "@jeffmanzione_language_tools//language-tools/lexer:token",
        "@jeffmanzione_language_tools//language-tools/parser",
    ],
)
