load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//zinnia:internal"],
)

cc_library(
    name = "scope",
    srcs = ["scope.c"],
    hdrs = ["scope.h"],
    deps = [
        ":isolates",
        "//zinnia/entity:object",
        "//zinnia/entity/class:classes",
        "//zinnia/entity/function",
        "//zinnia/entity/module",
        "//zinnia/entity/module:modules",
        "//zinnia/program:instruction",
        "//zinnia/program:bytecode",
        "//zinnia/vm:intern",
    ],
)

cc_library(
    name = "isolate",
    srcs = ["isolate.c"],
    hdrs = ["isolate.h"],
    deps = [
        ":isolates",
        ":frame",
        "//zinnia/alloc",
        "//zinnia/util/sync:critical_section",
        "//zinnia/util/sync:mutex",
        "@jeffmanzione_rzalloc//rzalloc",
    ],
)

cc_library(
    name = "isolates",
    srcs = ["isolates.c"],
    hdrs = ["isolates.h"],
    deps = [
        "//zinnia/entity:object",
        "//zinnia/entity/module",
        "//zinnia/heap",
        "//zinnia/program:bytecode",
        "//zinnia/util/sync:critical_section",
        "//zinnia/util/sync:mutex",
        "//zinnia/util/sync:thread",
        "//zinnia/util/sync:threadpool",
        "@jeffmanzione_c_data_structures//c-data-structures:arraylike",
        "@jeffmanzione_c_data_structures//c-data-structures:setlike",
        "@jeffmanzione_rzalloc//rzalloc",
    ],
)

cc_library(
    name = "frame",
    srcs = ["frame.c"],
    hdrs = ["frame.h"],
    deps = [
        ":scope",
        ":isolates",
        "//zinnia/entity/class:classes",
        "//zinnia/heap",
        "//zinnia/util:error",
    ],
)

cc_library(
    name = "actor_ref",
    srcs = ["actor_ref.c"],
    hdrs = ["actor_ref.h"],
    deps = [
        ":isolates",
        "//zinnia/alloc",
        "//zinnia/entity:value",
        "//zinnia/entity/class:classes",
        "//zinnia/heap",
    ],
)
