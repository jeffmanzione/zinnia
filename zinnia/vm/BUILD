load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "intern",
    srcs = ["intern.c"],
    hdrs = ["intern.h"],
    deps = [
        "@jeffmanzione_language_tools//language-tools:intern",
    ],
)

cc_library(
    name = "vm",
    srcs = ["vm.c"],
    hdrs = ["vm.h"],
    deps = [
        ":module_manager",
        "//zinnia/util/sync:mutex",
        "//zinnia/util/sync:threadpool",
        "//zinnia/vm/isolate",
        "//zinnia/vm/isolate:isolates",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
    ],
)

cc_library(
    name = "virtual_machine",
    srcs = ["virtual_machine.c"],
    hdrs = ["virtual_machine.h"],
    deps = [
        ":builtin_modules",
        ":module_manager",
        ":vm",
        "//zinnia/alloc",
        "//zinnia/value:object",
        "//zinnia/value/array",
        "//zinnia/value/native",
        "//zinnia/value/native:async",
        "//zinnia/value/native:builtin",
        "//zinnia/value/native:error",
        "//zinnia/value/native/builder",  # Force link
        "//zinnia/value/native/builder:function_context",
        "//zinnia/value/string",
        "//zinnia/value/string:string_helper",
        "//zinnia/value/tuple",
        "//zinnia/util/sync:mutex",
        "//zinnia/util/sync:thread",
        "//zinnia/vm:intern",
        "//zinnia/vm/isolate",
        "//zinnia/vm/isolate:scope",
        "//zinnia/vm/isolate:isolates",
        "//zinnia/vm/isolate:frame",
    ],
)

cc_library(
    name = "module_manager",
    srcs = ["module_manager.c"],
    hdrs = ["module_manager.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/value/class",
        "//zinnia/value/class:classes",
        "//zinnia/value/function",
        "//zinnia/value/module",
        "//zinnia/value/string:string_helper",
        "//zinnia/lang/lexer:lang_lexer",
        "//zinnia/lang/parser:lang_parser",
        "//zinnia/lang/semantic_analyzer:definitions",
        "//zinnia/program:bytecode",
        "//zinnia/program:bytecode_binary",
        "//zinnia/program/optimization:optimize",
        "//zinnia/util:dll",
        "//zinnia/util:file",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
        "@jeffmanzione_file_utils//file-utils:file_info",
        "@jeffmanzione_file_utils//file-utils:string_utils",
    ],
)

cc_library(
    name = "builtin_modules",
    srcs = ["builtin_modules.c"],
    hdrs = ["builtin_modules.h"],
    deps = [
        ":module_manager",
        "//zinnia/value/class:classes",
        "//zinnia/value/module",
        "//zinnia/value/module:modules",
        "//zinnia/value/native:async",
        "//zinnia/value/native:builtin",
        "//zinnia/value/native:classes",
        "//zinnia/value/native:data",
        "//zinnia/value/native:dynamic",
        "//zinnia/value/native:error",
        "//zinnia/value/native:io",
        "//zinnia/value/native:math",
        "//zinnia/value/native:socket",
        "//zinnia/value/native:time",
        "//zinnia/stdlib",
        "//zinnia/util:file",
        "//zinnia/vm:intern",
        "@jeffmanzione_file_utils//file-utils:file_utils",
        "@jeffmanzione_file_utils//file-utils:string_utils",
    ],
)
