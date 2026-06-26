load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "run",
    srcs = ["run.c"],
    hdrs = ["run.h"],
    deps = [
        "//zinnia/alloc",
        "//zinnia/compile",
        "//zinnia/entity/string:string_helper",
        "//zinnia/program/optimization:optimize",
        "//zinnia/seed",
        "//zinnia/util:error",
        "//zinnia/util:file",
        "//zinnia/util:void_array",
        "//zinnia/util/args:commandline",
        "//zinnia/util/args:commandlines",
        "//zinnia/util/args:lib_finder",
        "//zinnia/util/sync:constants",
        "//zinnia/util/sync:thread",
        "//zinnia/vm:module_manager",
        "//zinnia/vm:virtual_machine",
        "//zinnia/vm/process",
        "//zinnia/vm/process:processes",
        "//zinnia/vm/process:task",
        "@jeffmanzione_file_utils//file-utils:string_utils",
        "@jeffmanzione_language_tools//language-tools/parser",
        "@jeffmanzione_language_tools//language-tools/semantic_analyzer",
    ],
)
