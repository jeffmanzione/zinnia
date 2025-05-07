load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

VERSION = "local"

cc_library(
    name = "version_h",
    srcs = ["version.h"],
    deps = [
        "//util:time",
    ],
)

cc_library(
    name = "version_gen_main",
    srcs = ["version_gen_main.c"],
    deps = [
        "//util:time",
    ],
)

cc_binary(
    name = "version_gen_bin",
    deps = [":version_gen_main"],
)

genrule(
    name = "version_gen",
    outs = ["version.c"],
    cmd = "./$(location :version_gen_bin) \"" + VERSION + "\" > $(location :version.c)",
    tools = [":version_gen_bin"],
)

cc_library(
    name = "version",
    srcs = [":version_gen"],
    hdrs = ["version.h"],
    deps = ["//util:time"],
)
