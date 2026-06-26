load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "array",
    srcs = ["array.c"],
    hdrs = ["array.h"],
    deps = [
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/util:error",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
    ],
)
