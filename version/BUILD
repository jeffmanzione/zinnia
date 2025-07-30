load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("defs.bzl", "define_from_flag")

# Example compile command with version.
# bazel build -c opt //:zinnia --//version:version_flag=1.0.0

package(
    default_visibility = ["//visibility:public"],
)

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
        ":define_version_flag",
        "//util:time",
    ],
)

cc_binary(
    name = "version_gen_bin",
    deps = [":version_gen_main"],
)

define_from_flag(
    name = "define_version_flag",
    define = "VERSION",
    value = ":version_flag",
)

string_flag(
    name = "version_flag",
    build_setting_default = "local",
)

genrule(
    name = "version_gen",
    outs = ["version.c"],
    cmd = "./$(location :version_gen_bin) > $(location :version.c)",
    tools = [":version_gen_bin"],
)

cc_library(
    name = "version",
    srcs = [":version_gen"],
    hdrs = ["version.h"],
    deps = ["//util:time"],
)
