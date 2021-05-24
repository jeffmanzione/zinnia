load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "array",
    srcs = ["array.c"],
    hdrs = ["array.h"],
    deps = [
        "//entity",
        "//entity:object",
        "@c_data_structures//struct:arraylike",
        "@memory_wrapper//debug",
    ],
)
