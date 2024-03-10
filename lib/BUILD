load("//:zinnia_lang.bzl", "zinnia_library")

package(
    default_visibility = ["//visibility:public"],
)

filegroup(
    name = "lib_srcs",
    srcs = glob(["*.zn"]),
)

zinnia_library(
    name = "lib_zna_srcs",
    srcs = [":lib_srcs"],
    bin = False,
)

cc_library(
    name = "lib_gen_main",
    srcs = ["lib_gen_main.c"],
    deps = [
        "//util:string_util",
        "@file_utils//util/file:file_util",
        "@memory_wrapper//alloc",
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
