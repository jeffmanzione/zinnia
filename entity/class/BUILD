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
        "@memory_wrapper//alloc",
    ],
)

cc_library(
    name = "classes",
    srcs = ["classes.c"],
    hdrs = ["classes.h"],
    deps = [
        ":class",
        "//entity:object",
        "//entity/array",
        "//entity/function",
        "//entity/module",
        "//entity/string",
        "//entity/tuple",
        "//heap",
        "//heap:copy_fns",
        "//vm:intern",
        "@memory_wrapper//debug",
    ],
)
