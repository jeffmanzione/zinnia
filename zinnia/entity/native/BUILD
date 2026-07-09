load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "native_hdrs",
    hdrs = ["native_hdrs.h"],
    deps = [
        ":error",
        ":native",
        ":native_helpers",
        "//zinnia/alloc",
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/entity:primitive",
        "//zinnia/entity/array",
        "//zinnia/entity/class",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/function",
        "//zinnia/entity/string",
        "//zinnia/entity/string:string_helper",
        "//zinnia/entity/tuple",
        "//zinnia/heap",
        "//zinnia/util:error",
        "//zinnia/vm",
        "//zinnia/vm:intern",
        "//zinnia/vm:module_manager",
        "//zinnia/vm/process",
        "//zinnia/vm/process:context",
        "//zinnia/vm/process:processes",
        "//zinnia/vm/process:remote",
        "//zinnia/vm/process:task",
    ],
)

cc_library(
    name = "native",
    srcs = ["native.c"],
    hdrs = ["native.h"],
    deps = [
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/entity:primitive",
        "//zinnia/entity/class",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/function",
        "//zinnia/entity/string",
        "//zinnia/util:error",
        "//zinnia/vm/process:processes",
    ],
)

cc_library(
    name = "async",
    srcs = ["async.c"],
    hdrs = ["async.h"],
    deps = [
        ":native_hdrs",
        "//zinnia/util/sync:mutex",
        "//zinnia/util/sync:thread",
        "//zinnia/vm",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
    ],
)

cc_library(
    name = "builtin",
    srcs = ["builtin.c"],
    hdrs = ["builtin.h"],
    deps = [
        ":async",
        ":error",
        ":native",
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/entity:primitive",
        "//zinnia/entity/array",
        "//zinnia/entity/class",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/function",
        "//zinnia/entity/native:native_helpers",
        "//zinnia/entity/native/builder:function_context",
        "//zinnia/entity/string",
        "//zinnia/entity/string:string_helper",
        "//zinnia/entity/tuple",
        "//zinnia/heap",
        "//zinnia/util:error",
        "//zinnia/util:void_array",
        "//zinnia/vm",
        "//zinnia/vm:intern",
        "//zinnia/vm:module_manager",
        "//zinnia/vm/process:processes",
        "//zinnia/vm/process:remote",
        "//zinnia/vm/process:task",
        "@jeffmanzione_file_utils//file-utils:file_info",
    ],
)

cc_library(
    name = "native_helpers",
    hdrs = ["native_helpers.h"],
    deps = [
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/entity:primitive",
        "//zinnia/entity/native:error",
        "//zinnia/entity/tuple",
        "//zinnia/heap",
    ],
)

cc_library(
    name = "data",
    srcs = ["data.c"],
    hdrs = [
        "data.h",
        "data_helper.h",
    ],
    deps = [
        ":builtin",
        ":native_hdrs",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
    ],
)

cc_library(
    name = "io",
    srcs = ["io.c"],
    hdrs = ["io.h"],
    deps = [
        ":native_hdrs",
        "@jeffmanzione_file_utils//file-utils:file_utils",
    ],
)

cc_library(
    name = "error",
    srcs = ["error.c"],
    hdrs = ["error.h"],
    deps = [
        ":native",
        "//zinnia/alloc",
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/entity:primitive",
        "//zinnia/entity/array",
        "//zinnia/entity/class",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/function",
        "//zinnia/entity/string",
        "//zinnia/entity/string:string_helper",
        "//zinnia/entity/tuple",
        "//zinnia/util:error",
        "//zinnia/vm:intern",
        "//zinnia/vm:module_manager",
        "//zinnia/vm/process:processes",
        "//zinnia/vm/process:task",
    ],
)

cc_library(
    name = "math",
    srcs = ["math.c"],
    hdrs = ["math.h"],
    deps = [
        ":native_hdrs",
    ],
)

cc_library(
    name = "classes",
    srcs = ["classes.c"],
    hdrs = ["classes.h"],
    deps = [
        ":native_hdrs",
        "//zinnia/lang/parser:lang_parser",
        "//zinnia/lang/semantic_analyzer:definitions",
        "@jeffmanzione_file_utils//file-utils:file_info",
        "@jeffmanzione_language_tools//language-tools/parser",
        "@jeffmanzione_language_tools//language-tools/semantic_analyzer",
        "@jeffmanzione_language_tools//language-tools/semantic_analyzer:expression_tree",
    ],
)

cc_library(
    name = "socket",
    srcs = ["socket.c"],
    hdrs = ["socket.h"],
    deps = [
        ":native_hdrs",
        "//zinnia/util:socket",
    ],
)

cc_library(
    name = "dynamic",
    srcs = ["dynamic.c"],
    hdrs = ["dynamic.h"],
    deps = [
        ":native_hdrs",
        "//zinnia/util:dll",
    ],
)

cc_library(
    name = "time",
    srcs = ["time.c"],
    hdrs = ["time.h"],
    deps = [
        ":native_hdrs",
        "//zinnia/util:time",
    ],
)
