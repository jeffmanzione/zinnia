load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "builder",
    srcs = ["builder.c"],
    hdrs = ["builder.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":function_context",
        "//zinnia/entity/module",
        "//zinnia/vm:module_manager",
    ],
    alwayslink = True,
)

cc_library(
    name = "function_context",
    srcs = ["function_context.c"],
    hdrs = ["function_context.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//zinnia/entity",
        "//zinnia/entity:object",
        "//zinnia/entity/native:error",
        "//zinnia/entity/string:string_helper",
        "//zinnia/vm/process:processes",
    ],
    alwayslink = True,
)
