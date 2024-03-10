load("@rules_cc//cc:defs.bzl", "cc_library")
load("//:zinnia_lang.bzl", "zinnia_binary", "zinnia_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "_custom",
    srcs = ["_custom.c"],
    hdrs = ["_custom.h"],
    linkstatic = False,
    deps = [
        "//entity",
        "//entity:object",
        "//entity:primitive",
        "//entity/native",
        "//entity/native:error",
        "//vm:module_manager",
        "//vm/process:processes",
        "@memory_wrapper//debug",
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
    name = "dynamic_test",
    main = "dynamic_test.zn",
    deps = [":custom"],
)
