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
    name = "vm",
    srcs = ["vm.c"],
    hdrs = ["vm.h"],
    deps = [
        ":module_manager",
        "//util/sync:mutex",
        "//util/sync:threadpool",
        "//vm/process",
        "//vm/process:processes",
        "@c_data_structures//struct:alist",
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
        "//entity/string:string_helper",
        "//lang/parser",
        "//lang/semantics",
        "//program:tape",
        "//program:tape_binary",
        "//program/optimization:optimize",
        "@c_data_structures//struct:keyed_list",
        "@file_utils//util:string",
        "@file_utils//util/file:file_info",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//struct:set",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_library(
    name = "builtin_modules",
    srcs = ["builtin_modules.c"],
    hdrs = ["builtin_modules.h"],
    deps = [
        ":module_manager",
        "//entity/class:classes",
        "//entity/module",
        "//entity/module:modules",
        "//entity/native:async",
        "//entity/native:builtin",
        "//entity/native:classes",
        "//entity/native:dynamic",
        "//entity/native:error",
        "//entity/native:io",
        "//entity/native:math",
        "//entity/native:process",
        "//entity/native:socket",
        "//util:file",
        "@file_utils//util:string",
        "@file_utils//util/file:file_util",
        "@memory_wrapper//alloc/arena:intern",
    ],
)
