load("@language_tools//lang/parser:parser_builder.bzl", "parser_builder")

package(
    default_visibility = ["//visibility:public"],
)

parser_builder(
    name = "lang_parser",
    lexer = "//lang/lexer:lang_lexer",
    rules = "rules.txt",
)

cc_binary(
    name = "parser_test",
    srcs = ["parser_test.c"],
    data = ["//lib:lib_srcs"],
    deps = [
        ":lang_parser",
        "//lang/lexer:lang_lexer",
        "@c_data_structures//struct:q",
        "@file_utils//util/file:file_info",
        "@file_utils//util/file:sfile",
        "@language_tools//lang/lexer:token",
        "@language_tools//lang/parser",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
    ],
)
