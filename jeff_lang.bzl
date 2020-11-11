def _jeff_vm_library_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    compiler_executable_path = "./" + compiler_executable.short_path
    src_files = [file for target in ctx.attr.srcs for file in target.files.to_list()]
    out_files = [ctx.actions.declare_file(file.short_path.replace(".jl", ".ja")) for file in src_files]
    jlc_args = ["-a"] + [file.short_path for file in src_files]
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
            cfg = "host",
        ),
    },
)

def jeff_vm_library(
        name,
        srcs,
        deps = []):
    return _jeff_vm_library(name = name, srcs = srcs, deps = deps)
