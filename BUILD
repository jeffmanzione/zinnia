load("@rules_cc//cc:defs.bzl", "cc_binary")

package(
    default_visibility = ["//visibility:public"],
)

cc_binary(
    name = "zinnia",
    srcs = ["zinnia.c"],
    deps = ["//run"],
)

cc_binary(
    name = "zinniac",
    srcs = ["zinniac.c"],
    deps = ["//compile"],
)

cc_binary(
    name = "zinniap",
    srcs = ["zinniap.c"],
    deps = ["//package"],
)
