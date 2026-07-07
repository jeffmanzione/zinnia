load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "seed",
    srcs = ["seed.c"],
    hdrs = ["seed.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//zinnia/util:dll",
        "//zinnia/util:string_util",
        "//zinnia/vm",
        "@jeffmanzione_file_utils//file-utils:file_utils",
        "@jeffmanzione_file_utils//file-utils:string_utils",
        "@libzip",
    ],
)
