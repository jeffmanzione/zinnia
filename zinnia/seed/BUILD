load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "seed",
    srcs = ["seed.c"],
    hdrs = ["seed.h"],
    linkopts = [
        "-ldl",
        "-rdynamic",
    ],
    deps = [
        "//zinnia/util:string_util",
        "//zinnia/vm",
        "@jeffmanzione_file_utils//file-utils:file_utils",
        "@jeffmanzione_file_utils//file-utils:string_utils",
        "@libzip",
    ],
)
