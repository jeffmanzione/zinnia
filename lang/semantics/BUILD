load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "semantics",
    srcs = glob(["**/*.c"]),
    hdrs = glob(["**/*.h"]),
    deps = [
        "//lang/lexer:token",
        "//lang/parser",
        "//program:tape",
        "//struct:alist",
        "//struct:keyed_list",
        "//struct:q",
        "//vm:intern",
        "@memory-wrapper//alloc",
        "@memory-wrapper//alloc/arena:intern",
        "@memory-wrapper//debug",
        "@memory-wrapper//struct:map",
        "@memory-wrapper//struct:struct_defaults",
    ],
)
