load("@rules_cc//cc:defs.bzl", "cc_library")
load("//:zinnia_lang.bzl", "zinnia_binary", "zinnia_cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "pid_cc_lib",
    srcs = ["pid.c"],
    hdrs = ["pid.h"],
    deps = [
        "//entity",
        "//entity:object",
        "//entity/native",
        "//vm:module_manager",
        "//vm/process:processes",
    ],
)

zinnia_cc_library(
    name = "pid",
    cc_deps = [
        ":pid_cc_lib",
    ],
    cc_init_fn = "init_pid",
    src_module = "pid.zn",
)

zinnia_cc_library(
    name = "custom_math",
    cc_deps = [
        "//examples/dynamic:_custom",
    ],
    cc_init_fn = "_init_custom",
    src_module = "custom_math.zn",
)

zinnia_binary(
    name = "pid_test",
    cc_deps = [
        ":pid_cc_lib",
    ],
    main = "pid_test.zn",
    modules = [
        ":pid",
        ":custom_math",
    ],
)
