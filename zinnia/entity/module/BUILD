load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "module",
    srcs = ["module.c"],
    hdrs = ["module.h"],
    deps = [
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/entity/class",
        "//zinnia/entity/function",
        "//zinnia/heap",
        "//zinnia/program:tape",
        "//zinnia/util:dll",
        "@jeffmanzione_c_data_structures//c-data-structures:stable_maplike",
    ],
)

cc_library(
    name = "modules",
    srcs = ["modules.c"],
    hdrs = ["modules.h"],
    deps = [
        "//zinnia/entity:object",
    ],
)
