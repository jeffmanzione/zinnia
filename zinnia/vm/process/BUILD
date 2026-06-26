load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "context",
    srcs = ["context.c"],
    hdrs = ["context.h"],
    deps = [
        ":processes",
        "//zinnia/entity:object",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/function",
        "//zinnia/entity/module",
        "//zinnia/entity/module:modules",
        "//zinnia/program:instruction",
        "//zinnia/program:tape",
        "//zinnia/vm:intern",
    ],
)

cc_library(
    name = "process",
    srcs = ["process.c"],
    hdrs = ["process.h"],
    deps = [
        ":processes",
        ":task",
        "//zinnia/util/sync:critical_section",
        "//zinnia/util/sync:mutex",
        "@jeffmanzione_rzalloc//rzalloc",
    ],
)

cc_library(
    name = "processes",
    srcs = ["processes.c"],
    hdrs = ["processes.h"],
    deps = [
        "//zinnia/entity:object",
        "//zinnia/entity/module",
        "//zinnia/heap",
        "//zinnia/program:tape",
        "//zinnia/util/sync:critical_section",
        "//zinnia/util/sync:mutex",
        "//zinnia/util/sync:thread",
        "//zinnia/util/sync:threadpool",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_c_data_structures//c-data-structures:setlike",
        "@jeffmanzione_rzalloc//rzalloc",
    ],
)

cc_library(
    name = "task",
    srcs = ["task.c"],
    hdrs = ["task.h"],
    deps = [
        ":context",
        ":processes",
        "//zinnia/entity/class:classes",
        "//zinnia/heap",
        "//zinnia/util:error",
    ],
)

cc_library(
    name = "remote",
    srcs = ["remote.c"],
    hdrs = ["remote.h"],
    deps = [
        ":processes",
        "//zinnia/alloc",
        "//zinnia/entity",
        "//zinnia/entity/class:classes",
        "//zinnia/heap",
    ],
)
