load("@language_tools//lang/parser:parser_builder.bzl", "parser_builder")

package(
    default_visibility = ["//visibility:public"],
)

parser_builder(
    name = "lang_parser",
    lexer = "//lang/lexer",
    rules = "rules.txt",
)
