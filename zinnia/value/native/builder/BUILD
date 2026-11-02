load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "builder",
    srcs = ["builder.c"],
    hdrs = ["builder.h"],
    deps = [
        ":function_context",
        "//zinnia/value/module",
        "//zinnia/vm:module_manager",
    ],
    alwayslink = True,
)

cc_library(
    name = "function_context",
    srcs = ["function_context.c"],
    hdrs = ["function_context.h"],
    deps = [
        "//zinnia/value:value",
        "//zinnia/value:object",
        "//zinnia/value/native:error",
        "//zinnia/value/string:string_helper",
        "//zinnia/vm/isolate:isolates",
    ],
    alwayslink = True,
)
