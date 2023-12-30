load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "run",
    srcs = ["run.c"],
    hdrs = ["run.h"],
    deps = [
        "//compile",
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
        "@c_data_structures//struct:alist",
        "@file_utils//util:string",
        "@language_tools//lang/parser",
        "@language_tools//lang/semantic_analyzer",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)
