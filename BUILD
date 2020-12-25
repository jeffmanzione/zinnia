load(":jeff_lang.bzl", "jeff_vm_library")
load("@com_grail_bazel_compdb//:aspects.bzl", "compilation_database")

jeff_vm_library(
    name = "test",
    srcs = ["test.jl"],
)

compilation_database(
    name = "compdb",
    targets = [
        "//compile:jlc",
        "//run:jlr",
    ],
)
