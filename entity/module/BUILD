load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "module",
    srcs = ["module.c"],
    hdrs = ["module.h"],
    deps = [
        "//entity",
        "//entity:object",
        "//entity/class",
        "//entity/function",
        "//heap",
        "//program:tape",
        "@c_data_structures//struct:keyed_list",
    ],
)

cc_library(
    name = "modules",
    srcs = ["modules.c"],
    hdrs = ["modules.h"],
    deps = [
        "//entity:object",
    ],
)
