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
        "//vm:intern",
        "@c_data_structures//struct:alist",
        "@c_data_structures//struct:q",
        "@memory_wrapper//alloc",
        "@memory_wrapper//alloc/arena:intern",
        "@memory_wrapper//debug",
    ],
)
