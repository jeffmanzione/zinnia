# defs.bzl
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _define_from_flag_impl(ctx):
    return CcInfo(
        compilation_context = cc_common.create_compilation_context(
            defines = depset([
                "{}=\"{}\"".format(
                    ctx.attr.define,
                    ctx.attr.value[BuildSettingInfo].value,
                ),
            ]),
        ),
    )

define_from_flag = rule(
    implementation = _define_from_flag_impl,
    attrs = {
        "define": attr.string(),
        "value": attr.label(),
    },
)
