def _jeff_vm_library_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    compiler_executable_path = "./" + compiler_executable.short_path
    src_files = [file for target in ctx.attr.srcs for file in target.files.to_list()]
    out_files = [ctx.actions.declare_file(file.short_path.replace(".jl", ".ja")) for file in src_files]
    out_dir = out_files[0].dirname
    jlc_args = ["-a", "-aout=" + out_dir] + [file.short_path for file in src_files]
    ctx.actions.run(
        outputs = out_files,
        inputs = src_files,
        executable = compiler_executable,
        arguments = jlc_args,
        mnemonic = "CompileJL",
        progress_message = "Running command: %s %s" % (compiler_executable_path, " ".join(jlc_args)),
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
            default = Label("//compile:jlc"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
    },
)

def _prioritize_bin(file):
    if file.extension.endswith("jb"):
        return 0
    else:
        return 1

def jeff_vm_library(
        name,
        srcs):
    return _jeff_vm_library(name = name, srcs = srcs)

def _jeff_vm_binary_impl(ctx):
    runner_executable = ctx.attr.runner.files_to_run.executable
    builtins = [file for target in ctx.attr.builtins for file in target.files.to_list()]
    main_file = sorted(ctx.attr.main.files.to_list(), key = _prioritize_bin)[0]
    input_files = [file for target in ctx.attr.deps for file in target.files.to_list() if file.path.endswith(".ja")]
    input_files = [main_file] + input_files
    jlr_command = "%s %s" % (runner_executable.short_path, " ".join([file.short_path for file in input_files]))

    run_sh = ctx.actions.declare_file(ctx.label.name + ".sh")
    ctx.actions.run_shell(
        inputs = input_files,
        outputs = [run_sh],
        command = "echo \"%s\n\" > %s" % (jlr_command, run_sh.path),
    )
    return [
        DefaultInfo(
            files = depset(input_files + [runner_executable, run_sh]),
            executable = run_sh,
            default_runfiles = ctx.runfiles(files = input_files + [runner_executable, run_sh] + builtins),
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
            default = Label("//run:jlr"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "builtins": attr.label_list(),
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
    return _jeff_vm_binary(name = name, main = ":%s_main" % name, deps = deps, builtins = ["//lib"])
