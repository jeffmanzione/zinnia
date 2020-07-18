load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "test_lib",
    srcs = ["test.c"],
    data = [
        "lib/builtin.jl",
        "test.jl",
    ],
    deps = [
        "//lang/lexer",
        "//lang/lexer:file_info",
        "//lang/lexer:token",
        "//lang/parser",
        "//lang/semantics",
        "//program:instruction",
        "//program:op",
        "//program:tape",
        "//vm:module_manager",
        "//vm:virtual_machine",
        "//vm/process",
        "//vm/process:context",
        "//vm/process:processes",
        "//vm/process:task",
        "@memory-wrapper//alloc",
        "@memory-wrapper//alloc/arena:intern",
    ],
)

cc_binary(
    name = "test",
    deps = [
        ":test_lib",
    ],
)
