load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "run",
    srcs = ["run.c"],
    hdrs = ["run.h"],
    deps = [
        "//compile",
        "//lang/parser",
        "//lang/semantics",
        "//program/optimization:optimize",
        "//util:file",
        "//util/args:commandline",
        "//util/args:commandlines",
        "//util/args:lib_finder",
        "//util/sync:constants",
        "//util/sync:thread",
        "//vm:module_manager",
        "//vm:virtual_machine",
        "//vm/process",
        "//vm/process:processes",
        "//vm/process:task",
        "@file_utils//util:string",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)

cc_binary(
    name = "jvr",
    srcs = ["jvr.c"],
    data = ["//lib"],
    deps = [
        ":run",
    ],
)
