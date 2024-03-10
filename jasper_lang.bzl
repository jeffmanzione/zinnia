def _jasper_library_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    compiler_executable_path = "./" + compiler_executable.short_path
    src_files = [file for target in ctx.attr.srcs for file in target.files.to_list()]
    out_files = []
    for src in src_files:
        jasperc_args = []
        outputs = []

        if ctx.attr.assembly:
            a_out_file = ctx.actions.declare_file(src.basename.replace(".jp", ".ja"))
            out_files.append(a_out_file)
            out_dir = a_out_file.root.path + "/" + src.dirname
            jasperc_args.extend(["-a", "--assembly_out_dir=" + out_dir])
            outputs.append(a_out_file)

        if ctx.attr.bin:
            b_out_file = ctx.actions.declare_file(src.basename.replace(".jp", ".jb"))
            out_files.append(b_out_file)
            out_dir = b_out_file.root.path + "/" + src.dirname
            jasperc_args.extend(["-b", "--binary_out_dir=" + out_dir])
            outputs.append(b_out_file)

        jasperc_args.append(src.short_path)

        ctx.actions.run(
            outputs = outputs,
            inputs = [src],
            executable = compiler_executable,
            arguments = jasperc_args,
            mnemonic = "jasperc",
            progress_message = "Running command: %s %s" % (compiler_executable_path, " ".join(jasperc_args)),
        )
    return [
        DefaultInfo(
            files = depset(out_files),
        ),
        OutputGroupInfo(
            sources = depset(src_files),
        ),
    ]

_jasper_library = rule(
    implementation = _jasper_library_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            doc = "Source files",
        ),
        "deps": attr.label_list(),
        "compiler": attr.label(
            default = Label("//:jasperc"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "bin": attr.bool(
            default = True,
            doc = "Whether to output .jb files.",
        ),
        "assembly": attr.bool(
            default = True,
            doc = "Whether to output .ja files.",
        ),
    },
)

def _prioritize_bin(file):
    if file.extension.endswith("jb"):
        return 0
    else:
        return 1

def jasper_library(name, srcs, deps = [], bin = True, assembly = True):
    return _jasper_library(name = name, srcs = srcs, deps = deps, bin = bin, assembly = True)

def _jasper_binary_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    main_file = sorted(ctx.attr.main.files.to_list(), key = _prioritize_bin)[0]

    jpmodule_files = [file for target in ctx.attr.modules for file in target.files.to_list() if file.path.endswith(".jpmodule")]
    jpmodules_file = ctx.actions.declare_file(ctx.label.name + "_merged.jpmodule")

    if len(jpmodule_files) > 0:
        ctx.actions.run_shell(
            outputs = [jpmodules_file],
            inputs = jpmodule_files,
            command = "cat %s > %s" % (" ".join([file.path for file in jpmodule_files]), jpmodules_file.path),
        )
    else:
        ctx.actions.run_shell(
            outputs = [jpmodules_file],
            inputs = [],
            command = "touch %s" % jpmodules_file.path,
        )

    dep_files = [file for target in ctx.attr.deps for file in target.files.to_list() if file.path.endswith(".jp")] + [src for dep in ctx.attr.deps for src in dep[OutputGroupInfo].sources.to_list()]
    src_files = [jpmodules_file, main_file] + dep_files
    out_file = ctx.actions.declare_file(ctx.label.name + ".c")

    jasperp_args = [out_file.path] + [file.path for file in src_files]
    input_files = src_files

    ctx.actions.run(
        outputs = [out_file],
        inputs = input_files,
        executable = compiler_executable,
        arguments = jasperp_args,
        mnemonic = "jasperp",
        progress_message = "Running command: %s %s" %
                           (compiler_executable.path, " ".join(jasperp_args)),
    )
    return [
        DefaultInfo(
            files = depset([out_file]),
        ),
    ]

_jasper_binary = rule(
    implementation = _jasper_binary_impl,
    attrs = {
        "main": attr.label(
            doc = "Main file",
            allow_single_file = True,
            mandatory = True,
        ),
        "deps": attr.label_list(),
        "modules": attr.label_list(),
        "compiler": attr.label(
            default = Label("//:jasperp"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "builtins": attr.label_list(),
        "executable_ext": attr.string(default = ".sh"),
    },
)

def jasper_binary(name, main, srcs = [], deps = [], cc_deps = [], modules = []):
    if main in srcs:
        srcs.remove(main)
    deps = deps + [mdep + "_lib" for mdep in modules]
    if len(srcs) > 0:
        jasper_library(
            name = "%s_srcs" % name,
            srcs = srcs,
        )
        deps = [":%s_srcs" % name] + deps
    _jasper_binary(
        name = "%s_bin" % name,
        main = main,
        deps = deps,
        modules = modules,
    )
    native.cc_binary(
        name = name,
        srcs = [":%s_bin" % name],
        deps = cc_deps + [
            "//run",
            "//util/args:commandline",
            "//util/args:commandlines",
            "@c_data_structures//struct:alist",
            "@memory_wrapper//alloc",
            "@memory_wrapper//alloc/arena:intern",
        ],
    )

def _jasper_cc_library_impl(ctx):
    src_module = ctx.file.src_module
    out_file = ctx.actions.declare_file(ctx.label.name + ".jpmodule")
    cc_headers = [hdr for cc_dep in ctx.attr.cc_deps for hdr in cc_dep[CcInfo].compilation_context.direct_headers]
    ctx.actions.run_shell(
        outputs = [out_file],
        inputs = cc_headers,
        command = "echo \"%s:%s:%s\" > %s" % (src_module.path, ctx.attr.cc_init_fn, ",".join([hdr.path for hdr in cc_headers]), out_file.path),
    )
    return [DefaultInfo(files = depset([out_file]))]

_jasper_cc_library = rule(
    implementation = _jasper_cc_library_impl,
    attrs = {
        "src_module": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "Module file",
        ),
        "cc_deps": attr.label_list(),
        "cc_init_fn": attr.string(
            doc = "The function to call to initialize the module.",
        ),
    },
)

def jasper_cc_library(name, src_module, deps = [], cc_deps = [], cc_init_fn = None):
    jasper_library(
        name = "%s_lib" % name,
        srcs = [src_module],
        deps = deps,
    )
    return _jasper_cc_library(
        name = name,
        src_module = src_module,
        cc_deps = cc_deps,
        cc_init_fn = cc_init_fn,
    )
