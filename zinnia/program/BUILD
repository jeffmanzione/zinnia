load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "instruction",
    srcs = ["instruction.c"],
    hdrs = ["instruction.h"],
    deps = [
        ":op",
        "//zinnia/alloc",
        "//zinnia/entity:primitive",
        "//zinnia/util:error",
        "//zinnia/util:string_util",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
    ],
)

cc_library(
    name = "op",
    srcs = ["op.c"],
    hdrs = ["op.h"],
    deps = [],
)

cc_library(
    name = "bytecode",
    srcs = ["bytecode.c"],
    hdrs = ["bytecode.h"],
    deps = [
        ":instruction",
        "//zinnia/alloc",
        "//zinnia/lang/lexer:lang_lexer",
        "//zinnia/util:error",
        "//zinnia/util:string_util",
        "//zinnia/util:void_array",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_arraylike",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
        "@jeffmanzione_language_tools//language-tools/lexer:token",
    ],
)

cc_library(
    name = "bytecode_binary",
    srcs = ["bytecode_binary.c"],
    hdrs = ["bytecode_binary.h"],
    deps = [
        ":bytecode",
        "//zinnia/program/serialization:buffer",
        "//zinnia/program/serialization:deserialize",
        "//zinnia/program/serialization:serialize",
        "//zinnia/util:void_array",
        "//zinnia/vm:intern",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
    ],
)
