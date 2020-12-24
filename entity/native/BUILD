load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "native",
    srcs = ["native.c"],
    hdrs = ["native.h"],
    deps = [
        "//entity",
        "//entity:object",
        "//entity:primitive",
        "//entity/class",
        "//entity/class:classes",
        "//entity/function",
        "//vm/process:processes",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "async",
    srcs = ["async.c"],
    hdrs = ["async.h"],
    deps = [
        ":native",
        "//entity",
        "//entity:object",
        "//entity/class:classes",
        "//heap",
        "//vm/process:processes",
        "//vm/process:task",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "builtin",
    srcs = ["builtin.c"],
    hdrs = ["builtin.h"],
    deps = [
        ":error",
        ":native",
        "//entity",
        "//entity:object",
        "//entity:primitive",
        "//entity/array",
        "//entity/class",
        "//entity/class:classes",
        "//entity/function",
        "//entity/string",
        "//entity/string:string_helper",
        "//entity/tuple",
        "//heap",
        "//lang/lexer:file_info",
        "//vm/process:processes",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
        "@memory_wrapper//util",
    ],
)

cc_library(
    name = "io",
    srcs = ["io.c"],
    hdrs = ["io.h"],
    deps = [
        ":error",
        ":native",
        "//entity",
        "//entity:object",
        "//entity:primitive",
        "//entity/array",
        "//entity/class",
        "//entity/class:classes",
        "//entity/function",
        "//entity/string",
        "//entity/string:string_helper",
        "//entity/tuple",
        "//vm/process:processes",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "error",
    srcs = ["error.c"],
    hdrs = ["error.h"],
    deps = [
        ":native",
        "//entity",
        "//entity:object",
        "//entity:primitive",
        "//entity/array",
        "//entity/class",
        "//entity/class:classes",
        "//entity/function",
        "//entity/string",
        "//entity/string:string_helper",
        "//entity/tuple",
        "//vm/process:processes",
        "//vm/process:task",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "math",
    srcs = ["math.c"],
    hdrs = ["math.h"],
    deps = [
        ":error",
        ":native",
        "//entity",
        "//entity:object",
        "//entity:primitive",
        "//entity/class",
        "//entity/class:classes",
        "//entity/tuple",
        "//vm/process:processes",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)
