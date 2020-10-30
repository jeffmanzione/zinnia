load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "compile",
    srcs = ["compile.c"],
    hdrs = ["compile.h"],
    deps = [
        "//lang/parser",
        "//lang/semantics",
        "//program:tape",
        "//program:tape_binary",
        "//util:file",
        "//util:string",
        "//util/args:commandline",
        "//util/args:commandlines",
        "//util/args:lib_finder",
        "//vm:module_manager",
        "//vm:virtual_machine",
        "@memory_wrapper//alloc",
        "@memory_wrapper//debug",
        "@memory_wrapper//struct:map",
        "@memory_wrapper//struct:set",
        "@memory_wrapper//struct:struct_defaults",
    ],
)

cc_binary(
    name = "jlc",
    srcs = ["jlc.c"],
    deps = [":compile"],
)
