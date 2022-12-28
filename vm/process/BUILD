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
        "//util/sync:critical_section",
        "//util/sync:mutex",
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
        "//util/sync:critical_section",
        "//util/sync:mutex",
        "//util/sync:thread",
        "//util/sync:threadpool",
        "@c_data_structures//struct:alist",
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
        "//heap",
        "@c_data_structures//struct:alist",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_library(
    name = "remote",
    srcs = ["remote.c"],
    hdrs = ["remote.h"],
    deps = [
        ":processes",
        "//entity",
        "//entity/class:classes",
        "//heap",
    ],
)
