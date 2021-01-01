load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "intern",
    srcs = ["intern.c"],
    hdrs = ["intern.h"],
    deps = [
        "@memory_wrapper//alloc/arena:intern",
    ],
)

cc_library(
    name = "virtual_machine",
    srcs = ["virtual_machine.c"],
    hdrs = ["virtual_machine.h"],
    deps = [
        ":module_manager",
        "//entity:object",
        "//entity/array",
        "//entity/native",
        "//entity/native:async",
        "//entity/native:builtin",
        "//entity/native:error",
        "//entity/string",
        "//entity/string:string_helper",
        "//entity/tuple",
        "//util/sync:mutex",
        "//util/sync:thread",
        "//vm/process",
        "//vm/process:context",
        "//vm/process:processes",
        "//vm/process:task",
        "@memory_wrapper//alloc/arena:intern",
    ],
)

cc_library(
    name = "module_manager",
    srcs = ["module_manager.c"],
    hdrs = ["module_manager.h"],
    deps = [
        "//entity/class",
        "//entity/class:classes",
        "//entity/function",
        "//entity/module",
        "//entity/module:modules",
        "//entity/native:async",
        "//entity/native:builtin",
        "//entity/native:error",
        "//entity/native:io",
        "//entity/native:math",
        "//lang/parser",
        "//lang/semantics",
        "//program:tape",
        "//program:tape_binary",
        "//program/optimization:optimize",
        "//struct:keyed_list",
        "@file_utils//util:string",
        "@file_utils//util/file:file_info",
    ],
)
