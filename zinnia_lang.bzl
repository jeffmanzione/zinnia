def _zinnia_library_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    compiler_executable_path = "./" + compiler_executable.short_path
    src_files = [file for target in ctx.attr.srcs for file in target.files.to_list()]
    out_files = []
    for src in src_files:
        zinniac_args = []
        outputs = []

        if ctx.attr.assembly:
            a_out_file = ctx.actions.declare_file(src.basename.replace(".zn", ".zna"))
            out_files.append(a_out_file)
            out_dir = a_out_file.root.path + "/" + src.dirname
            zinniac_args.extend(["-a", "--assembly_out_dir=" + out_dir])
            outputs.append(a_out_file)

        if ctx.attr.bin:
            b_out_file = ctx.actions.declare_file(src.basename.replace(".zn", ".znb"))
            out_files.append(b_out_file)
            out_dir = b_out_file.root.path + "/" + src.dirname
            zinniac_args.extend(["-b", "--binary_out_dir=" + out_dir])
            outputs.append(b_out_file)

        zinniac_args.append(src.short_path)

        ctx.actions.run(
            outputs = outputs,
            inputs = [src],
            executable = compiler_executable,
            arguments = zinniac_args,
            mnemonic = "CompileZN",
            progress_message = "Running command: %s %s" % (compiler_executable_path, " ".join(zinniac_args)),
        )
    return [
        DefaultInfo(
            files = depset(out_files),
        ),
    ]

_zinnia_library = rule(
    implementation = _zinnia_library_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            doc = "Source files",
        ),
        "deps": attr.label_list(),
        "compiler": attr.label(
            default = Label("//:zinniac"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "bin": attr.bool(
            default = True,
            doc = "Whether to output .znb files.",
        ),
        "assembly": attr.bool(
            default = True,
            doc = "Whether to output .zna files.",
        ),
    },
)

def _prioritize_bin(file):
    if file.extension.endswith(".znb"):
        return 0
    else:
        return 1

def zinnia_library(name, srcs, bin = True, assembly = True):
    return _zinnia_library(name = name, srcs = srcs, bin = bin, assembly = True)

def _zinnia_binary_impl(ctx):
    runner_executable = ctx.attr.runner.files_to_run.executable
    main_file = sorted(ctx.attr.main.files.to_list(), key = _prioritize_bin)[0]
    input_files = [file for target in ctx.attr.deps for file in target.files.to_list() if file.path.endswith(".zna")]
    input_files = [main_file] + input_files
    zinnia_command = "./%s %s" % (runner_executable.short_path, " ".join([file.short_path for file in input_files]))

    run_sh = ctx.actions.declare_file(ctx.label.name + ".sh")
    ctx.actions.write(
        output = run_sh,
        is_executable = True,
        content = zinnia_command,
    )
    return [
        DefaultInfo(
            files = depset(input_files + [runner_executable, run_sh]),
            executable = run_sh,
            default_runfiles = ctx.runfiles(files = input_files + [runner_executable, run_sh]),
        ),
    ]

_zinnia_binary = rule(
    implementation = _zinnia_binary_impl,
    attrs = {
        "main": attr.label(
            doc = "Main file",
        ),
        "deps": attr.label_list(),
        "runner": attr.label(
            default = Label("//:zinnia"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "builtins": attr.label_list(),
        "executable_ext": attr.string(default = ".sh"),
    },
    executable = True,
)

def zinnia_binary(name, main, srcs = [], deps = []):
    if main in srcs:
        srcs.remove(main)

    zinnia_library(
        "%s_main" % name,
        srcs = [main],
    )
    if len(srcs) > 0:
        zinnia_library(
            "%s_srcs" % name,
            srcs = srcs,
        )
        deps = [":%s_srcs" % name] + deps
    return _zinnia_binary(
        name = name,
        main = ":%s_main" % name,
        deps = deps,
    )
