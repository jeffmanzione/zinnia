load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "codegen",
    srcs = ["codegen.c"],
    hdrs = ["codegen.h"],
    deps = [
        "//util:string_util",
        "@file_utils//util/file:file_util",
        "@memory_wrapper//alloc",
    ],
)

cc_library(
    name = "platform",
    hdrs = ["platform.h"],
    deps = [],
)

cc_library(
    name = "file",
    srcs = ["file.c"],
    hdrs = ["file.h"],
    deps = [
        ":platform",
        "@c_data_structures//struct:alist",
        "@file_utils//util/file:file_util",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
    ],
)

cc_library(
    name = "string_util",
    srcs = ["string_util.c"],
    hdrs = ["string_util.h"],
    deps = [
        "@memory_wrapper//alloc",
    ],
)

cc_library(
    name = "socket",
    srcs = ["socket.c"],
    hdrs = ["socket.h"],
    linkopts = select({
        "@bazel_tools//src/conditions:windows": [
            "-DEFAULTLIB:ws2_32.lib",
            "-DEFAULTLIB:wsock32.lib",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":platform",
        # "//util/sync:mutex",
        "@memory_wrapper//alloc",
    ],
)

cc_library(
    name = "time",
    srcs = ["time.c"],
    hdrs = ["time.h"],
    deps = [],
)
