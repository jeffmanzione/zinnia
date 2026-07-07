load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//zinnia:zinnia.bzl", "zinnia_binary", "zinnia_cc_library", "zinnia_library", "zinnia_seed")

cc_library(
    name = "hello_c",
    srcs = ["hello.c"],
    hdrs = ["hello.h"],
    deps = [
        "//zinnia:native",
    ],
    alwayslink = True,
)

zinnia_cc_library(
    name = "hello_lib",
    cc_deps = [":hello_c"],
    cc_init_fn = "init_hello",
    src_module = "hello.zn",
)

zinnia_library(
    name = "greeter_lib",
    srcs = ["greeter.zn"],
)

zinnia_seed(
    name = "hello",
    deps = [
        ":greeter_lib",
        ":hello_lib",
    ],
)

zinnia_binary(
    name = "run_hello",
    main = "run_hello.zn",
    deps = [":hello"],
)
