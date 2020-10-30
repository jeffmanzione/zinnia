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
        "//util:file",
        "//util:string",
        "//util/args:commandline",
        "//util/args:commandlines",
        "//util/args:lib_finder",
        "//vm:module_manager",
        "//vm:virtual_machine",
        "//vm/process",
        "//vm/process:processes",
        "//vm/process:task",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
    ],
)

cc_binary(
    name = "jlr",
    srcs = ["jlr.c"],
    deps = [":run"],
)
