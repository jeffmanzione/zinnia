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
        "@c_data_structures//struct:keyed_list",
        "@memory_wrapper//alloc",
    ],
)

cc_library(
    name = "classes_def",
    hdrs = [
        "class.h",
        "classes_def.h",
    ],
    deps = [
        "//entity:object",
    ],
)

cc_library(
    name = "classes",
    srcs = ["classes.c"],
    hdrs = [
        "classes.h",
        "classes_def.h",
    ],
    deps = [
        ":class",
        ":classes_def",
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
