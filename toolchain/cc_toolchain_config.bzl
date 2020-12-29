# NEW
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")

# NEW
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "artifact_name_pattern",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)

all_link_actions = [
    # NEW
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "C:/MinGW/bin/gcc",
        ),
        tool_path(
            name = "ld",
            path = "C:/MinGW/bin/ld",
        ),
        tool_path(
            name = "ar",
            path = "C:/MinGW/bin/ar",
        ),
        tool_path(
            name = "cpp",
            path = "C:/MinGW/bin/cpp",
        ),
        tool_path(
            name = "gcov",
            path = "C:/MinGW/bin/gcov",
        ),
        tool_path(
            name = "nm",
            path = "C:/MinGW/bin/nm",
        ),
        tool_path(
            name = "objdump",
            path = "C:/MinGW/bin/objdump",
        ),
        tool_path(
            name = "strip",
            path = "C:/MinGW/bin/strip",
        ),
    ]

    features = [
        # NEW
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-lstdc++",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,  # NEW
        cxx_builtin_include_directories = [
            "C:/MinGW/include",
            "C:/MinGW/mingw32/include",
            "C:/MinGW/lib/gcc/mingw32/4.7.2/include-fixed",
            "C:/MinGW/lib/gcc/mingw32/4.7.2/include",
            "C:/MinGW/lib/gcc/mingw32/4.7.2",
        ],
        toolchain_identifier = "local",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "x64_windows",
        target_libc = "unknown",
        compiler = "gcc",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
        artifact_name_patterns = [
            artifact_name_pattern(
                category_name = "executable",
                prefix = "",
                extension = ".exe",
            ),
        ],
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
