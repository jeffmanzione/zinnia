load("//:jeff_lang.bzl", "jeff_vm_binary")

package(
    default_visibility = ["//visibility:public"],
)

jeff_vm_binary(
    name = "builder",
    main = "builder.jv",
)

jeff_vm_binary(
    name = "mvc",
    main = "mvc.jv",
)
