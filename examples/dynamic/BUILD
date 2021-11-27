load("@rules_cc//cc:defs.bzl", "cc_library")
load("//:jeff_lang.bzl", "jeff_vm_binary", "jeff_vm_library")

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

jeff_vm_library(
    name = "custom",
    srcs = ["custom.jv"],
)

jeff_vm_binary(
    name = "test",
    main = "test.jv",
    deps = [":custom"],
)
