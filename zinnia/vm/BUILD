load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
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
        "//zinnia/vm/process",
        "//zinnia/vm/process:processes",
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
        "//zinnia/entity:object",
        "//zinnia/entity/array",
        "//zinnia/entity/native",
        "//zinnia/entity/native:async",
        "//zinnia/entity/native:builtin",
        "//zinnia/entity/native:error",
        "//zinnia/entity/string",
        "//zinnia/entity/string:string_helper",
        "//zinnia/entity/tuple",
        "//zinnia/util/sync:mutex",
        "//zinnia/util/sync:thread",
        "//zinnia/vm/process",
        "//zinnia/vm/process:context",
        "//zinnia/vm/process:processes",
        "//zinnia/vm/process:task",
        "@jeffmanzione_language_tools//language-tools:intern",
    ],
)

cc_library(
    name = "module_manager",
    srcs = ["module_manager.c"],
    hdrs = ["module_manager.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/entity/class",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/function",
        "//zinnia/entity/module",
        "//zinnia/entity/string:string_helper",
        "//zinnia/lang/lexer:lang_lexer",
        "//zinnia/lang/parser:lang_parser",
        "//zinnia/lang/semantic_analyzer:definitions",
        "//zinnia/program:tape",
        "//zinnia/program:tape_binary",
        "//zinnia/program/optimization:optimize",
        "//zinnia/util:file",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
        "@jeffmanzione_file_utils//file-utils:file_info",
        "@jeffmanzione_file_utils//file-utils:string_utils",
        "@jeffmanzione_language_tools//language-tools:intern",
    ],
)

cc_library(
    name = "builtin_modules",
    srcs = ["builtin_modules.c"],
    hdrs = ["builtin_modules.h"],
    deps = [
        ":module_manager",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/module",
        "//zinnia/entity/module:modules",
        "//zinnia/entity/native:async",
        "//zinnia/entity/native:builtin",
        "//zinnia/entity/native:classes",
        "//zinnia/entity/native:data",
        "//zinnia/entity/native:dynamic",
        "//zinnia/entity/native:error",
        "//zinnia/entity/native:io",
        "//zinnia/entity/native:math",
        "//zinnia/entity/native:socket",
        "//zinnia/entity/native:time",
        "//zinnia/lib",
        "//zinnia/util:file",
        "@jeffmanzione_file_utils//file-utils:file_utils",
        "@jeffmanzione_file_utils//file-utils:string_utils",
        "@jeffmanzione_language_tools//language-tools:intern",
    ],
)
