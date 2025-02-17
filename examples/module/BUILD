load("//:zinnia_lang.bzl", "zinnia_binary")

package(
    default_visibility = ["//visibility:public"],
)

zinnia_binary(
    name = "module",
    srcs = glob(["**/*.*"]),
    main = "module.zn",
)
