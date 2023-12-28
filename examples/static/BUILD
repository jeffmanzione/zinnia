load("@rules_cc//cc:defs.bzl", "cc_library")
load("//:jasper_lang.bzl", "jasper_binary", "jasper_cc_library")

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

jasper_cc_library(
    name = "pid",
    cc_deps = [
        ":pid_cc_lib",
    ],
    cc_init_fn = "init_pid",
    src_module = "pid.jp",
)

jasper_binary(
    name = "pid_test",
    cc_deps = [
        ":pid_cc_lib",
    ],
    main = "pid_test.jp",
    modules = [
        ":pid",
    ],
)
