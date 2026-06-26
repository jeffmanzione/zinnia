load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "buffer",
    srcs = ["buffer.c"],
    hdrs = ["buffer.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/util:error",
    ],
)

cc_library(
    name = "serialize",
    srcs = ["serialize.c"],
    hdrs = ["serialize.h"],
    deps = [
        ":buffer",
        "//zinnia/alloc",
        "//zinnia/entity:primitive",
        "//zinnia/program:instruction",
        "//zinnia/util:error",
        "@jeffmanzione_c_data_structures//c-data-structures:maplike",
    ],
)

cc_library(
    name = "deserialize",
    srcs = ["deserialize.c"],
    hdrs = ["deserialize.h"],
    deps = [
        ":buffer",
        "//zinnia/entity:primitive",
        "//zinnia/program:instruction",
        "//zinnia/util:error",
        "//zinnia/util:void_array",
    ],
)
