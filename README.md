# jeff-vm

A process virtual machine that I've created in my spare time.

The VM is written from scratch by me in C and meets the C90 standard. It compiles on Linux (via gcc) and Windows (via MSVC). I've heavily leveraged [Bazel](https://bazel.build/) to take advantage of incremental and fast builds.

## Dependencies

[bazel](https://bazel.build/) - Compile and building the entire application.
[language-tools](https://github.com/jeffreymanzione/language-tools) - Creating the lexer, parser, and semanatic analyzer.
[memory-wrapper](https://github.com/jeffreymanzione/memory-wrapper) - Heap, intern, arena, and memory debugging.
[file-utils](https://github.com/jeffreymanzione/file-utils) - Basic C file wrapper for reading files.
[c-data-structures](https://github.com/jeffreymanzione/c-data-structures) - Some useful data structures.

## Downloading and building this project

| :exclamation: You will need to install Bazel to build the binaries. Follow the [Bazel installation instructions](https://bazel.build/install). I promise it is very easy to install! |
|-|

To build jeff-vm:

```shell
# Clones this git repository.
git clone https://github.com/jeffreymanzione/jeff-vm.git

# Bazel requires building within the workspace.
cd jeff-vm

# Builds the compiler and runner.
bazel build -c opt //compile:jlc //run:jlr
```

| (Optional :100:) You can copy `./bazel-bin/compile/jvc` and `./bazel-bin/run/jvr` to a location on your `PATH` to make it easier to reference these binaries. Set your environment variable, `JV_LIB_PATH`, to point to the `lib/` directory in the git repo. This allows the built-in libraries to be used.|
|-|

## Compiling your program to assembly and bytecode

Use `jlc` to compile your program.

* `-a`: Output assembly (default=`false`).
* `-b`: Output binary (default=`false`).
* `-o`: Optimize the program (default=`true`).
* `-binary_out_dir`: Output location of JB files (default=`"./"`).
* `-assembly_out_dir`: Output location for JA files (default=`"./"`).

```shell
./bazel-bin/compile/jlc -a -b my_program.jv
```

## Running your program

Use `jlr` to run your program either from source, assembly, or bytecode.

```shell
./bazel-bin/run/jlr my_program.jv
# or
./bazel-bin/run/jlr my_program.ja
# or
./bazel-bin/run/jlr my_program.jb
```
