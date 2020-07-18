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
        "//entity/module",
        "//program:instruction",
        "//program:tape",
    ],
)

cc_library(
    name = "process",
    srcs = ["process.c"],
    hdrs = ["process.h"],
    deps = [
        ":processes",
        ":task",
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
        "@memory-wrapper//struct:set",
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
    ],
)
