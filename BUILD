load(":jeff_lang.bzl", "jeff_vm_binary")

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
)

jeff_vm_binary(
    name = "hello",
    main = "hello.jl",
)
