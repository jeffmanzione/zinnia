load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "commandline",
    srcs = ["commandline.c"],
    hdrs = ["commandline.h"],
    deps = [
        ":commandline_arg",
        "//zinnia/alloc",
        "//zinnia/util:error",
        "//zinnia/util:void_array",
        "//zinnia/version",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
        "@jeffmanzione_c_data_structures//c-data-structures:setlike",
    ],
)

cc_library(
    name = "commandlines",
    srcs = ["commandlines.c"],
    hdrs = ["commandlines.h"],
    deps = [
        ":commandline",
        ":commandline_arg",
        ":lib_finder",
        "//zinnia/util:error",
    ],
)

cc_library(
    name = "commandline_arg",
    srcs = ["commandline_arg.c"],
    hdrs = ["commandline_arg.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/util:error",
        "//zinnia/vm:intern",
    ],
)

cc_library(
    name = "lib_finder",
    srcs = ["lib_finder.c"],
    hdrs = ["lib_finder.h"],
    deps = [],
)
