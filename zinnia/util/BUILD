load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "codegen",
    srcs = ["codegen.c"],
    hdrs = ["codegen.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/util:string_util",
        "@jeffmanzione_file_utils//file-utils:file_utils",
    ],
)

cc_library(
    name = "platform",
    hdrs = ["platform.h"],
)

cc_library(
    name = "file",
    srcs = ["file.c"],
    hdrs = ["file.h"],
    deps = [
        ":platform",
        "//zinnia/alloc",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_file_utils//file-utils:file_utils",
    ],
)

cc_library(
    name = "string_util",
    srcs = ["string_util.c"],
    hdrs = ["string_util.h"],
    deps = [
        ":error",
        "//zinnia/alloc",
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
        # "//zinnia/util/sync:mutex",
        "//zinnia/alloc",
        ":error",
    ],
)

cc_library(
    name = "time",
    srcs = ["time.c"],
    hdrs = ["time.h"],
)

cc_library(
    name = "error",
    srcs = ["error.c"],
    hdrs = ["error.h"],
)

cc_library(
    name = "void_array",
    srcs = ["void_array.c"],
    hdrs = ["void_array.h"],
    deps = [
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_c_data_structures//c-data-structures:setlike",
    ],
)
