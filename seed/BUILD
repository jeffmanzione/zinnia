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
        "//util:string_util",
        "//vm",
        "@file_utils//util/file:file_util",
        "@libzip",
    ],
)
