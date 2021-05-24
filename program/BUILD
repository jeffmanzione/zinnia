load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "instruction",
    srcs = ["instruction.c"],
    hdrs = ["instruction.h"],
    deps = [
        ":op",
        "//entity:primitive",
        "//util:string_util",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "op",
    srcs = ["op.c"],
    hdrs = ["op.h"],
    deps = [],
)

cc_library(
    name = "tape",
    srcs = ["tape.c"],
    hdrs = ["tape.h"],
    deps = [
        ":instruction",
        "//lang/lexer:token",
        "@c_data_structures//struct:alist",
        "@c_data_structures//struct:keyed_list",
        "@c_data_structures//struct:q",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "tape_binary",
    srcs = ["tape_binary.c"],
    hdrs = ["tape_binary.h"],
    deps = [
        ":tape",
        "//program/serialization:buffer",
        "//program/serialization:deserialize",
        "//program/serialization:serialize",
        "@c_data_structures//struct:alist",
        "@c_data_structures//struct:keyed_list",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:struct_defaults",
    ],
)
