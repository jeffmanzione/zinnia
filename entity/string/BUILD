load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "string",
    srcs = ["string.c"],
    hdrs = ["string.h"],
    deps = [
        "//entity",
        "//entity:object",
        "@c_data_structures//struct:arraylike",
        "@memory_wrapper//debug",
    ],
)

cc_library(
    name = "string_helper",
    srcs = ["string_helper.c"],
    hdrs = ["string_helper.h"],
    deps = [
        ":string",
        "//entity:object",
        "//entity/class:classes",
        "//heap",
        "@memory_wrapper//debug",
    ],
)
