load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "optimize",
    srcs = ["optimize.c"],
    hdrs = ["optimize.h"],
    deps = [
        ":optimizer",
        ":optimizers",
        "//zinnia/entity",
        "//zinnia/entity:primitive",
        "//zinnia/program:instruction",
        "//zinnia/program:op",
        "//zinnia/util:error",
        "//zinnia/util:void_array",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_language_tools//language-tools/lexer:token",
    ],
)

cc_library(
    name = "optimizer",
    srcs = ["optimizer.c"],
    hdrs = ["optimizer.h"],
    deps = [
        "//zinnia/program:instruction",
        "//zinnia/program:tape",
        "//zinnia/util:error",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
    ],
)

cc_library(
    name = "optimizers",
    srcs = ["optimizers.c"],
    hdrs = ["optimizers.h"],
    deps = [
        ":optimizer",
        "//zinnia/alloc",
        "//zinnia/entity",
        "//zinnia/program:instruction",
        "//zinnia/program:tape",
        "//zinnia/vm:intern",
        "@jeffmanzione_language_tools//language-tools:intern",
    ],
)
