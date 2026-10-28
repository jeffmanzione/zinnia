load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "bundle",
    srcs = ["bundle.c"],
    hdrs = ["bundle.h"],
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
