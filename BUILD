load(":jeff_lang.bzl", "jeff_vm_binary", "jeff_vm_library")

package(
    default_visibility = ["//visibility:public"],
)

filegroup(
    name = "lib",
    srcs = glob(["lib/*.jl"]),
)

jeff_vm_binary(
    name = "test",
    main = "test.jl",
    deps = [],
)

jeff_vm_library(
    name = "hello",
    srcs = ["hello.jl"],
)
