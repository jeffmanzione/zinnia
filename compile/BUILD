load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "compile",
    srcs = ["compile.c"],
    hdrs = ["compile.h"],
    deps = [
        "//lang/parser:lang_parser",
        "//program:tape",
        "//program:tape_binary",
        "//program/optimization:optimize",
        "//util/args:commandline",
        "//util/args:commandlines",
        "//util/args:lib_finder",
        "//vm:module_manager",
        "@c_data_structures//struct:q",
        "@file_utils//util:string",
        "@file_utils//util/file:file_info",
        "@file_utils//util/file:file_util",
        "@language_tools//lang/lexer:token",
        "@language_tools//lang/parser",
        "@language_tools//lang/semantic_analyzer",
        "@language_tools//lang/semantic_analyzer:expression_tree",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:set",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_binary(
    name = "jasperc",
    srcs = ["jasperc.c"],
    deps = [":compile"],
)
