load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "buffer",
    srcs = ["buffer.c"],
    hdrs = ["buffer.h"],
    deps = [
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "serialize",
    srcs = ["serialize.c"],
    hdrs = ["serialize.h"],
    deps = [
        ":buffer",
        "//entity:primitive",
        "//program:instruction",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
    ],
)

cc_library(
    name = "deserialize",
    srcs = ["deserialize.c"],
    hdrs = ["deserialize.h"],
    deps = [
        ":buffer",
        "//entity:primitive",
        "//program:instruction",
        "@c_data_structures//struct:alist",
        "@memory_wrapper//debug",
    ],
)
