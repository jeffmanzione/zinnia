load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "context",
    srcs = ["context.c"],
    hdrs = ["context.h"],
    deps = [
        ":processes",
        "//entity:object",
        "//entity/class:classes",
        "//entity/function",
        "//entity/module",
        "//entity/module:modules",
        "//program:instruction",
        "//program:tape",
        "//vm:intern",
    ],
)

cc_library(
    name = "process",
    srcs = ["process.c"],
    hdrs = ["process.h"],
    deps = [
        ":processes",
        ":task",
        "@memory_wrapper//alloc/arena",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_library(
    name = "processes",
    hdrs = ["processes.h"],
    deps = [
        "//entity:object",
        "//entity/module",
        "//heap",
        "//program:tape",
        "//struct:alist",
        "@memory_wrapper//struct:set",
    ],
)

cc_library(
    name = "task",
    srcs = ["task.c"],
    hdrs = ["task.h"],
    deps = [
        ":context",
        ":processes",
        "//entity/class:classes",
        "//struct:alist",
        "@memory_wrapper//debug",
    ],
)
