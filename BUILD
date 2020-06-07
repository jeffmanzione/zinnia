load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "test_lib",
    srcs = ["test.c"],
    deps = [
        "//lang/lexer",
        "//lang/lexer:file_info",
        "//lang/lexer:token",
        "//lang/parser",
        "//program:instruction",
        "//program:op",
        "//program:tape",
        "@memory-wrapper//alloc",
        "@memory-wrapper//alloc/arena:intern",
    ],
)

cc_binary(
    name = "test",
    defines = ["DEBUG"],
    deps = [
        ":test_lib",
    ],
)
