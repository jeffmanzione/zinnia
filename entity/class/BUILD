load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "class",
    srcs = ["class.c"],
    hdrs = ["class.h"],
    deps = [
        "//entity:object",
        "//entity/function",
        "//struct:keyed_list",
        "@memory-wrapper//alloc",
    ],
)

cc_library(
    name = "classes",
    srcs = ["classes.c"],
    hdrs = ["classes.h"],
    deps = [
        "//entity:object",
        "//entity/array",
        "//entity/class",
        "//entity/function",
        "//entity/module",
        "//entity/string",
        "//entity/tuple",
        "//heap",
        "//vm:intern",
        "@memory-wrapper//debug",
    ],
)
