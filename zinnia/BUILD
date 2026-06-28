load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(
    default_visibility = ["//visibility:private"],
)

package_group(
    name = "internal",
    packages = ["//zinnia/..."],
)

cc_binary(
    name = "zinnia",
    srcs = ["zinnia.c"],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/run"],
)

cc_binary(
    name = "zinniac",
    srcs = ["zinniac.c"],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/compile"],
)

cc_binary(
    name = "zinniap",
    srcs = ["zinniap.c"],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/package"],
)

cc_binary(
    name = "zinnias",
    srcs = ["zinnias.c"],
    visibility = ["//visibility:public"],
    deps = ["//zinnia/seed"],
)
