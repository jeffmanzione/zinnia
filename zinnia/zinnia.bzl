"""Rules for the zinnia programming language."""

load("@bazel_skylib//lib:shell.bzl", "shell")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

ZinniaModuleInfo = provider(
    doc = "Contains metadata about zinnia module.",
    fields = [
        "znmodule_file",
        "srcs",
        "init_fn",
        "cc_deps",
    ],
)

ZinniaLibraryInfo = provider(
    doc = "Contains metadata about zinnia library.",
    fields = [
        "srcs",
    ],
)

ZinniaBundleInfo = provider(
    doc = "Contains metadata about zinnia bundle.",
    fields = [
        "bundle_file",
    ],
)

def _zinnia_library_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    compiler_executable_path = "./" + compiler_executable.short_path
    src_files = [file for target in ctx.attr.srcs for file in target.files.to_list()]
    out_files = []
    for src in src_files:
        zinniac_args = []
        outputs = []

        # zinniac writes <out_dir>/<basename>.zn{a,b}, so declare outputs at the
        # source's path relative to this package to keep subdirectories intact.
        package_prefix = ctx.label.package + "/" if ctx.label.package else ""
        if src.short_path.startswith(package_prefix):
            out_base = src.short_path[len(package_prefix):]
        else:
            out_base = src.basename
        out_base = out_base[:-len(src.extension) - 1] if src.extension else out_base

        if ctx.attr.assembly:
            a_out_file = ctx.actions.declare_file(out_base + ".zna")
            out_files.append(a_out_file)
            zinniac_args.append("-a")
            if ctx.attr.minimize:
                zinniac_args.append("-m")
            zinniac_args.append("--assembly_out_dir=" + a_out_file.dirname)
            outputs.append(a_out_file)

        if ctx.attr.bin:
            b_out_file = ctx.actions.declare_file(out_base + ".znb")
            out_files.append(b_out_file)
            zinniac_args.extend(["-b", "--binary_out_dir=" + b_out_file.dirname])
            outputs.append(b_out_file)

        zinniac_args.append(src.short_path)

        ctx.actions.run(
            outputs = outputs,
            inputs = [src],
            executable = compiler_executable,
            arguments = zinniac_args,
            mnemonic = "zinniac",
            progress_message = "Running command: %s %s" % (compiler_executable_path, " ".join(zinniac_args)),
        )

    return [
        DefaultInfo(
            files = depset(out_files),
            runfiles = ctx.runfiles(files = ctx.files.data),
        ),
        ZinniaLibraryInfo(
            srcs = depset(src_files),
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
            default = Label("//zinnia:zinniac"),
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
        "minimize": attr.bool(
            default = False,
            doc = "Whether the .zna output should be minimzed.",
        ),
        "data": attr.label_list(
            allow_files = True,
            doc = "Data",
        ),
    },
)

def _prioritize_bin(file):
    if file.extension.endswith("znb"):
        return 0
    else:
        return 1

def zinnia_library(name, srcs, deps = [], bin = True, assembly = True, minimize = False, data = []):
    """Generates compiled files for the zinnia programming langyage.

    Args:
        name: Name of the target.
        srcs: zinnia sources.
        deps: zinnia_library dependencies.
        bin: Whether binary (.znb) files should be generated.
        assembly: Whether assembly (.zna) files should be generated.
        minimize: Whether asembly should be minimized.
        data: The data needed at runtime.
    """
    _zinnia_library(name = name, srcs = srcs, deps = deps, bin = bin, assembly = assembly, minimize = minimize, data = data)

def _zinnia_binary_impl(ctx):
    compiler_executable = ctx.attr.compiler.files_to_run.executable
    main_file = sorted(ctx.attr.main.files.to_list(), key = _prioritize_bin)[0]

    znmodule_files = [file for target in ctx.attr.modules for file in target.files.to_list() if file.path.endswith(".znmodule")]
    znmodules_file = ctx.actions.declare_file(ctx.label.name + "_merged.znmodule")

    if len(znmodule_files) > 0:
        ctx.actions.run_shell(
            outputs = [znmodules_file],
            inputs = znmodule_files,
            command = "cat %s > %s" % (" ".join([file.path for file in znmodule_files]), znmodules_file.path),
        )
    else:
        ctx.actions.run_shell(
            outputs = [znmodules_file],
            inputs = [],
            command = "touch %s" % znmodules_file.path,
        )

    dep_files = []
    for dep in ctx.attr.deps:
        if ZinniaLibraryInfo in dep:
            for src in dep[ZinniaLibraryInfo].srcs.to_list():
                dep_files.append(src)
        if ZinniaBundleInfo in dep:
            dep_files.append(dep[ZinniaBundleInfo].bundle_file)

    src_files = [znmodules_file, main_file] + dep_files
    out_file = ctx.actions.declare_file(ctx.label.name + ".c")

    package_args = [out_file.path] + [file.path for file in src_files]
    input_files = src_files

    ctx.actions.run(
        outputs = [out_file],
        inputs = input_files,
        executable = compiler_executable,
        arguments = package_args,
        mnemonic = "ZinniaPackage",
        progress_message = "Running command: %s %s" %
                           (compiler_executable.path, " ".join(package_args)),
    )
    return [
        DefaultInfo(
            files = depset([out_file]),
            runfiles = ctx.runfiles(files = ctx.files.data),
        ),
    ]

_zinnia_binary = rule(
    implementation = _zinnia_binary_impl,
    attrs = {
        "main": attr.label(
            doc = "Main file",
            allow_single_file = True,
            mandatory = True,
        ),
        "deps": attr.label_list(),
        "data": attr.label_list(
            allow_files = True,
            doc = "Data",
        ),
        "modules": attr.label_list(),
        "compiler": attr.label(
            default = Label("//zinnia:zinnia-package"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "builtins": attr.label_list(),
    },
)

def zinnia_binary(name, main, srcs = [], deps = [], cc_deps = [], modules = [], data = []):
    """Compiles an executable binary for a zinnia program.

    Args:
        name: Name of the binary.
        main: The zinnia file to serve as main().
        srcs: Additional sources to the main program.
        deps: Zinnia libraries that should be included in the binary.
        cc_deps: C (cc_library) targets that should be included in the binary.
        modules: Modules (zinnia_cc_library) targets that should be included in the binary.
        data: Data needed by the program.
    """
    if main in srcs:
        srcs.remove(main)
    deps = deps + [mdep + "_lib" for mdep in modules]
    cc_deps = cc_deps + [mdep + "_lib_cc_deps" for mdep in modules]
    if len(srcs) > 0:
        zinnia_library(
            name = "%s_srcs" % name,
            srcs = srcs,
            data = data,
        )
        deps = [":%s_srcs" % name] + deps
    _zinnia_binary(
        name = "%s_bin" % name,
        main = main,
        deps = deps,
        data = data,
        modules = modules,
    )
    cc_binary(
        name = name,
        srcs = [":%s_bin" % name],
        data = data,
        deps = cc_deps + [
            Label("//zinnia/run"),
            Label("//zinnia/util/args:commandline"),
            Label("//zinnia/util/args:commandlines"),
            Label("//zinnia/alloc"),
        ],
    )

def _zinnia_cc_library_impl(ctx):
    src_module = ctx.file.src_module
    out_file = ctx.actions.declare_file(ctx.label.name + ".znmodule")
    cc_headers = [hdr for cc_dep in ctx.attr.cc_deps for hdr in cc_dep[CcInfo].compilation_context.direct_headers]
    ctx.actions.run_shell(
        outputs = [out_file],
        inputs = cc_headers,
        command = "echo \"%s:%s:%s\" > %s" % (src_module.path, ctx.attr.cc_init_fn, ",".join([hdr.path for hdr in cc_headers]), out_file.path),
    )
    return [
        DefaultInfo(
            files = depset([out_file]),
            runfiles = ctx.runfiles(files = ctx.files.data),
        ),
        ZinniaLibraryInfo(
            srcs = depset([src_module]),
        ),
        ZinniaModuleInfo(
            znmodule_file = out_file,
            init_fn = ctx.attr.cc_init_fn,
            cc_deps = depset(ctx.attr.cc_deps),
        ),
    ]

_zinnia_cc_library = rule(
    implementation = _zinnia_cc_library_impl,
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
        "data": attr.label_list(
            allow_files = True,
            doc = "Data",
        ),
    },
)

def zinnia_cc_library(name, src_module, deps = [], cc_deps = [], cc_init_fn = None, data = []):
    """Generates the files necessary to statically link C used with zinnia code into a zinna_binary() as a module.

    Args:
        name: Name of the target.
        src_module: The .zn file used as the source.
        deps: zinnia_library() deps needed by src_module.
        cc_deps: C (cc_library) targets that should be included in the binary.
        cc_init_fn: The C function name that should be called to initiate the library.
        data: Data needed at runtime.
    """
    zinnia_library(
        name = "%s_lib" % name,
        srcs = [src_module],
        deps = deps,
        data = data,
    )
    cc_library(
        name = "%s_lib_cc_deps" % name,
        deps = cc_deps,
        data = data,
    )
    _zinnia_cc_library(
        name = name,
        src_module = src_module,
        cc_deps = cc_deps,
        cc_init_fn = cc_init_fn,
        data = data,
    )

def _zinnia_bundle_impl(ctx):
    bundle_executable = ctx.attr.bundle_executable.files_to_run.executable
    input_files = []

    bundle_name = ctx.label.name
    bundle_file = ctx.actions.declare_file("%s.znbundle" % bundle_name)

    args = [
        bundle_name,
        bundle_file.path,
    ]

    for dep in ctx.attr.deps:
        if ZinniaModuleInfo in dep:
            # Should only ever be 1 source file for modules.
            src = dep[ZinniaLibraryInfo].srcs.to_list()[0]
            module_name = src.path.replace(".zn", "").replace(".zna", "").replace(".znb", "")
            src_input_path = src.path
            input_files.append(src)
            src_output_path = src.short_path
            lib = [
                lib[OutputGroupInfo].dynamic_library.to_list()
                for lib in dep[ZinniaModuleInfo].cc_deps.to_list()
            ][0][0]
            lib_input_path = lib.path
            input_files.append(lib)
            lib_output_path = lib.short_path
            init_fn = dep[ZinniaModuleInfo].init_fn
            args.append("%s:%s:%s:%s:%s:%s" % (module_name, src_input_path, src_output_path, lib_input_path, lib_output_path, init_fn))
        elif ZinniaLibraryInfo in dep:
            for src in dep[ZinniaLibraryInfo].srcs.to_list():
                args.append("%s:%s:%s" % (src.path.replace(".zn", "").replace(".zna", "").replace(".znb", ""), src.path, src.short_path))
                input_files.append(src)

    ctx.actions.run(
        outputs = [bundle_file],
        inputs = input_files,
        executable = bundle_executable,
        arguments = args,
        mnemonic = "ZinniaBundle",
        progress_message = "Running command: %s %s" %
                           (bundle_executable.path, " ".join(args)),
    )
    return [
        DefaultInfo(
            files = depset([bundle_file]),
        ),
        ZinniaBundleInfo(
            bundle_file = bundle_file,
        ),
    ]

_zinnia_bundle = rule(
    implementation = _zinnia_bundle_impl,
    attrs = {
        "bundle_executable": attr.label(
            default = Label("//zinnia:zinnia-bundle"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
        "deps": attr.label_list(),
    },
)

def zinnia_bundle(name, deps = []):
    """Bundles zinnia libraries and modules into a .znbundle archive.

    Args:
        name: Name of the bundle.
        deps: zinnia_library() and zinnia_cc_library() targets to include.
    """
    return _zinnia_bundle(
        name = name,
        deps = deps,
    )

def zinnia_seed(name, deps = []):
    """Deprecated: use zinnia_bundle()."""
    return zinnia_bundle(name = name, deps = deps)

def _zinnia_test_impl(ctx):
    runner_executable = ctx.attr.runner.files_to_run.executable
    main_file = sorted(ctx.attr.main.files.to_list(), key = _prioritize_bin)[0]

    test_script_file = ctx.actions.declare_file(ctx.label.name + ".sh")

    dep_files = []
    for dep in ctx.attr.deps:
        if ZinniaLibraryInfo in dep:
            for src in dep[ZinniaLibraryInfo].srcs.to_list():
                dep_files.append(src)
        if ZinniaBundleInfo in dep:
            dep_files.append(dep[ZinniaBundleInfo].bundle_file)

    src_files = [main_file] + dep_files

    # The test passes iff the program exits with code 0. Programs exit nonzero
    # on an uncaught error, `exit` with a nonzero value, or a failed
    # test.Tester().test().
    ctx.actions.write(
        output = test_script_file,
        content = "#!/bin/bash\nexec {} {} \"$@\"\n".format(
            shell.quote(runner_executable.short_path),
            " ".join([shell.quote(file.short_path) for file in src_files]),
        ),
        is_executable = True,
    )

    runfiles = ctx.runfiles(files = [runner_executable] + src_files + ctx.files.data)
    runfiles = runfiles.merge(ctx.attr.runner[DefaultInfo].default_runfiles)

    return [
        DefaultInfo(
            executable = test_script_file,
            runfiles = runfiles,
        ),
    ]

_zinnia_test = rule(
    implementation = _zinnia_test_impl,
    attrs = {
        "main": attr.label(
            doc = "Main file",
            allow_single_file = True,
            mandatory = True,
        ),
        "deps": attr.label_list(),
        "data": attr.label_list(
            allow_files = True,
            doc = "Data",
        ),
        "runner": attr.label(
            default = Label("//zinnia:zinnia"),
            executable = True,
            allow_single_file = True,
            cfg = "target",
        ),
    },
    test = True,
)

def zinnia_test(name, main, srcs = [], deps = [], modules = [], data = []):
    """Runs a Zinnia test.

    Args:
        name: Name of the test.
        main: The zinnia file to serve as main().
        srcs: Additional sources to the main program.
        deps: Zinnia libraries that should be included in the binary.
        modules: Modules (zinnia_cc_library) targets that should be included in the binary.
        data: Data needed by the program.
    """
    if main in srcs:
        srcs.remove(main)
    deps = deps + [mdep + "_lib" for mdep in modules]
    if len(srcs) > 0:
        zinnia_library(
            name = "%s_srcs" % name,
            srcs = srcs,
            data = data,
        )
        deps = [":%s_srcs" % name] + deps
    _zinnia_test(
        name = name,
        main = main,
        deps = deps,
        data = data,
    )
