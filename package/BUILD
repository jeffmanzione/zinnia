load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "package",
    srcs = ["package.c"],
    hdrs = ["package.h"],
    deps = [
        "//compile",
        "//program/optimization:optimize",
        "//util:codegen",
        "//util:file",
        "//util:string_util",
        "//version",
        "//vm:intern",
        "@file_utils//util:string",
        "@file_utils//util/file:file_util",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)
