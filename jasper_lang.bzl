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

def jasper_library(name, srcs, bin = True, assembly = True):
    return _jasper_library(name = name, srcs = srcs, bin = bin, assembly = True)

def _jasper_binary_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    main_file = sorted(ctx.attr.main.files.to_list(), key = _prioritize_bin)[0]
    dep_files = [file for target in ctx.attr.deps for file in target.files.to_list() if file.path.endswith(".ja")]
    input_files = [main_file] + dep_files
    out_file = ctx.actions.declare_file(ctx.label.name + ".c")
    jasperp_args = [out_file.path] + [file.path for file in input_files]

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

def jasper_binary(name, main, srcs = [], deps = []):
    if main in srcs:
        srcs.remove(main)
    if len(srcs) > 0:
        jasper_library(
            "%s_srcs" % name,
            srcs = srcs,
        )
        deps = [":%s_srcs" % name] + deps
    _jasper_binary(
        name = "%s_bin" % name,
        main = main,
        deps = deps,
    )
    native.cc_binary(
        name = name,
        srcs = [":%s_bin" % name],
        deps = [
            "//run",
            "//util/args:commandline",
            "//util/args:commandlines",
            "@c_data_structures//struct:alist",
            "@memory_wrapper//alloc",
            "@memory_wrapper//alloc/arena:intern",
        ],
    )
