load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "test_lib",
    srcs = ["test.c"],
    data = [
        "lib/builtin.jl",
        "lib/io.jl",
        "test.jl",
    ],
    deps = [
        "//lang/parser",
        "//lang/semantics",
        "//vm:module_manager",
        "//vm:virtual_machine",
        "//vm/process",
        "//vm/process:context",
        "//vm/process:processes",
        "//vm/process:task",
        "@memory_wrapper//alloc",
    ],
)

cc_binary(
    name = "test",
    deps = [
        ":test_lib",
    ],
)
