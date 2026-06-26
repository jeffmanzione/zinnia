load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(
    default_visibility = ["//visibility:public"],
)

cc_binary(
    name = "zinnia",
    srcs = ["zinnia.c"],
    deps = ["//zinnia/run"],
)

cc_binary(
    name = "zinniac",
    srcs = ["zinniac.c"],
    deps = ["//zinnia/compile"],
)

cc_binary(
    name = "zinniap",
    srcs = ["zinniap.c"],
    deps = ["//zinnia/package"],
)

cc_binary(
    name = "zinnias",
    srcs = ["zinnias.c"],
    deps = ["//zinnia/seed"],
)
