load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "package",
    srcs = ["package.c"],
    hdrs = ["package.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/compile",
        "//zinnia/program/optimization:optimize",
        "//zinnia/seed",
        "//zinnia/util:codegen",
        "//zinnia/util:error",
        "//zinnia/util:file",
        "//zinnia/util:string_util",
        "//zinnia/util:void_array",
        "//zinnia/version",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
        "@jeffmanzione_file_utils//file-utils:file_utils",
        "@jeffmanzione_file_utils//file-utils:string_utils",
        "@jeffmanzione_language_tools//language-tools:intern",
    ],
)
