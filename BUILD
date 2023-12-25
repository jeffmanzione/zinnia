load("@rules_cc//cc:defs.bzl", "cc_binary")

package(
    default_visibility = ["//visibility:public"],
)

cc_binary(
    name = "jasper",
    srcs = ["jasper.c"],
    deps = ["//run"],
)

cc_binary(
    name = "jasperc",
    srcs = ["jasperc.c"],
    deps = ["//compile"],
)

cc_binary(
    name = "jasperp",
    srcs = ["jasperp.c"],
    deps = ["//package"],
)
