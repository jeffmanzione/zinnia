load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//zinnia:zinnia.bzl", "zinnia_library")

package(
    default_visibility = ["//zinnia:internal"],
)

filegroup(
    name = "stdlib_srcs",
    srcs = glob(["*.zn"]),
)

zinnia_library(
    name = "stdlib_zna_srcs",
    srcs = [":stdlib_srcs"],
    bin = False,
    minimize = True,
)

cc_library(
    name = "stdlib_gen_main",
    srcs = ["stdlib_gen_main.c"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/util:codegen",
        "@jeffmanzione_file_utils//file-utils:file_utils",
    ],
)

cc_binary(
    name = "stdlib_gen_bin",
    deps = [":stdlib_gen_main"],
)

genrule(
    name = "stdlib_gen",
    srcs = [
        ":stdlib_zna_srcs",
    ],
    outs = ["lib.h"],
    cmd = "./$(location :stdlib_gen_bin) $(location :lib.h) $(locations :stdlib_zna_srcs)",
    tools = [":stdlib_gen_bin"],
)

cc_library(
    name = "stdlib",
    hdrs = [":stdlib_gen"],
)
