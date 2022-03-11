load("//:jeff_lang.bzl", "jeff_vm_library")

package(
    default_visibility = ["//visibility:public"],
)

filegroup(
    name = "lib_srcs",
    srcs = glob(["*.jv"]),
)

jeff_vm_library(
    name = "lib",
    srcs = [":lib_srcs"],
)
