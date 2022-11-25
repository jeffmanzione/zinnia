def _jeff_vm_library_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    compiler_executable_path = "./" + compiler_executable.short_path
    src_files = [file for target in ctx.attr.srcs for file in target.files.to_list()]
    out_files = []
    for src in src_files:
        a_out_file = ctx.actions.declare_file(src.basename.replace(".jv", ".ja"))
        out_files.append(a_out_file)
        b_out_file = None
        out_dir = a_out_file.root.path + "/" + src.dirname
        jvc_args = [
            "-a",
            "--assembly_out_dir=" + out_dir,
        ]
        outputs = [a_out_file]

        if ctx.attr.bin:
            b_out_file = ctx.actions.declare_file(src.basename.replace(".jv", ".jb"))
            out_files.append(b_out_file)
            jvc_args.extend(["-b", "--binary_out_dir=" + out_dir])
            outputs.append(b_out_file)

        jvc_args.append(src.short_path)

        ctx.actions.run(
            outputs = outputs,
            inputs = [src],
            executable = compiler_executable,
            arguments = jvc_args,
            mnemonic = "CompileJL",
            progress_message = "Running command: %s %s" % (compiler_executable_path, " ".join(jvc_args)),
        )
    return [
        DefaultInfo(
            files = depset(out_files),
        ),
    ]

_jeff_vm_library = rule(
    implementation = _jeff_vm_library_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            doc = "Source files",
        ),
        "deps": attr.label_list(),
        "compiler": attr.label(
            default = Label("//compile:jvc"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "bin": attr.bool(
            default = True,
            doc = "Whether to output .jb files.",
        ),
    },
)

def _prioritize_bin(file):
    if file.extension.endswith("jb"):
        return 0
    else:
        return 1

def jeff_vm_library(name, srcs, bin = True):
    return _jeff_vm_library(name = name, srcs = srcs, bin = bin)

def _jeff_vm_binary_impl(ctx):
    runner_executable = ctx.attr.runner.files_to_run.executable
    main_file = sorted(ctx.attr.main.files.to_list(), key = _prioritize_bin)[0]
    input_files = [file for target in ctx.attr.deps for file in target.files.to_list() if file.path.endswith(".ja")]
    input_files = [main_file] + input_files
    jvr_command = "./%s %s" % (runner_executable.short_path, " ".join([file.short_path for file in input_files]))

    run_sh = ctx.actions.declare_file(ctx.label.name + ".sh")
    ctx.actions.write(
        output = run_sh,
        is_executable = True,
        content = jvr_command,
    )
    return [
        DefaultInfo(
            files = depset(input_files + [runner_executable, run_sh]),
            executable = run_sh,
            default_runfiles = ctx.runfiles(files = input_files + [runner_executable, run_sh]),
        ),
    ]

_jeff_vm_binary = rule(
    implementation = _jeff_vm_binary_impl,
    attrs = {
        "main": attr.label(
            doc = "Main file",
        ),
        "deps": attr.label_list(),
        "runner": attr.label(
            default = Label("//run:jvr"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "builtins": attr.label_list(),
        "executable_ext": attr.string(default = ".sh"),
    },
    executable = True,
)

def jeff_vm_binary(name, main, srcs = [], deps = []):
    if main in srcs:
        srcs.remove(main)

    jeff_vm_library(
        "%s_main" % name,
        srcs = [main],
    )
    if len(srcs) > 0:
        jeff_vm_library(
            "%s_srcs" % name,
            srcs = srcs,
        )
        deps = [":%s_srcs" % name] + deps
    return _jeff_vm_binary(
        name = name,
        main = ":%s_main" % name,
        deps = deps,
    )
