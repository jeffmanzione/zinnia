load("@rules_cc//cc:cc_library.bzl", "cc_library")
# load("//:zinnia_lang.bzl", "zinnia_binary", "zinnia_cc_library", "zinnia_library", "zinnia_seed")

cc_library(
    name = "hello_c",
    srcs = ["hello.c"],
    hdrs = ["hello.h"],
    deps = [
        "//entity",
        "//entity:object",
        "//entity/native",
        "//entity/native:error",
        "//entity/string:string_helper",
        "//vm",
        "//vm:module_manager",
        "//vm/process:processes",
        "//vm/process:task",
        "@memory_wrapper//alloc/arena:intern",
    ],
)

# zinnia_cc_library(
#     name = "hello_lib",
#     cc_deps = [":hello_c"],
#     cc_init_fn = "init_hello",
#     src_module = "hello.zn",
# )

# zinnia_library(
#     name = "greeter_lib",
#     srcs = ["greeter.zn"],
# )

# zinnia_seed(
#     name = "hello",
#     deps = [
#         ":greeter_lib",
#         ":hello_lib",
#     ],
# )

# zinnia_binary(
#     name = "run_hello",
#     main = "run_hello.zn",
#     deps = [":hello"],
# )
