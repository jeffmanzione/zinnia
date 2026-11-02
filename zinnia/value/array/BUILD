load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "array",
    srcs = ["array.c"],
    hdrs = ["array.h"],
    deps = [
        "//zinnia/value:value",
        "//zinnia/value:object",
        "//zinnia/util:error",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
    ],
)
