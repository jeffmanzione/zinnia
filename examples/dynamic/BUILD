load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//zinnia:zinnia.bzl", "zinnia_binary", "zinnia_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "_custom",
    srcs = ["_custom.c"],
    hdrs = ["_custom.h"],
    deps = [
        "//zinnia:native",
    ],
    alwayslink = True,
)

cc_binary(
    name = "_custom.dll",
    linkshared = 1,
    deps = [":_custom"],
)

zinnia_library(
    name = "custom",
    srcs = ["custom.zn"],
)

zinnia_binary(
    name = "dynamic",
    cc_deps = [
        ":_custom.dll",
    ],
    main = "dynamic.zn",
    deps = [":custom"],
)
