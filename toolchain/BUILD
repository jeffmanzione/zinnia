load("@rules_cc//cc:defs.bzl", "cc_toolchain", "cc_toolchain_suite")
load(":cc_toolchain_config.bzl", "cc_toolchain_config", "mingw_directories")

package(default_visibility = ["//visibility:public"])

cc_toolchain_suite(
    name = "toolchains",
    toolchains = {
        "x64_windows": ":mingw_toolchain",
    },
)

filegroup(name = "empty")

cc_toolchain(
    name = "mingw_toolchain",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
    toolchain_config = ":mingw_toolchain_config",
    toolchain_identifier = "mingw-toolchain",
)

cc_toolchain_config(
    name = "mingw_toolchain_config",
    builtin_include_directories = mingw_directories("4.7.2"),
)
