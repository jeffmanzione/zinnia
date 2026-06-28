load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//zinnia:zinnia.bzl", "zinnia_library")

package(
    default_visibility = ["//zinnia:internal"],
)

filegroup(
    name = "lib_srcs",
    srcs = glob(["*.zn"]),
)

zinnia_library(
    name = "lib_zna_srcs",
    srcs = [":lib_srcs"],
    bin = False,
    minimize = True,
)

cc_library(
    name = "lib_gen_main",
    srcs = ["lib_gen_main.c"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/util:codegen",
        "@jeffmanzione_file_utils//file-utils:file_utils",
    ],
)

cc_binary(
    name = "lib_gen_bin",
    deps = [":lib_gen_main"],
)

genrule(
    name = "lib_gen",
    srcs = [
        ":lib_zna_srcs",
    ],
    outs = ["lib.h"],
    cmd = "./$(location :lib_gen_bin) $(location :lib.h) $(locations :lib_zna_srcs)",
    tools = [":lib_gen_bin"],
)

cc_library(
    name = "lib",
    hdrs = [":lib_gen"],
)
