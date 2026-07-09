
![ZInnia Logo](https://repository-images.githubusercontent.com/266423086/01b55ccc-4cd2-4d80-a260-24ffc896c6f4)

## A process virtual machine programming language that melds my favorite features features of JavaScript and Java.

## Why use Zinnia?

I don't recommend it... yet. It's still an exploration.

It supports compilation of programs to the following outputs:

- **Executable** (.exe): A native executable file for the program that can be run without Zinnia.
- **Intermediate language** (.zna): An intermediate program representation of the source that is optimized.
- **Bytecode** (.znb): A bytecode representation of the program.
- **Archive** (.znseed): Archives that can be unpacked by Zinnia at runtime.

.zna, .znb, and .znseed files can be run by the `zinnia` executable.

## Dependencies

- [bazel](https://bazel.build/) - Compile and building the entire application.
- [c-data-structures](https://github.com/jeffmanzione/c-data-structures) - Some useful data structures.
- [file-utils](https://github.com/jeffmanzione/file-utils) - Basic C file wrapper for reading files.
- [intern](https://github.com/jeffmanzione/intern) - String intern.
- [language-tools](https://github.com/jeffmanzione/language-tools) - Creating the lexer, parser, and semanatic analyzer.
- [libzip](https://github.com/nih-at/libzip) - For .zip creation.
- [memory-wrapper](https://github.com/jeffmanzione/memory-wrapper) - Heap-based memory storage.
- [rzalloc](https://github.com/jeffmanzione/rzalloc) - Bulk memory allocation.

## Downloading this project

Download the latest binary from the [Releases](https://github.com/jeffmanzione/zinnia/releases) page.

## Building this project from source

**Note**: Requires Bazel 8+. Follow the [Bazel installation instructions](https://bazel.build/install) to install Bazel.

To build zinnia binaries from source:

```shell
# Clone this git repository
git clone https://github.com/jeffmanzione/zinnia.git && cd zinnia

args=(
    //zinnia          # Runner
    //zinnia:zinniac  # Compiler
    //zinnia:zinniap  # Packager
    //zinnia:zinnias  # Seed bundler
)
# Build above binaries
bazel build -c opt "${args[@]}"
```

## Compiling your program to assembly and bytecode

Use `zinniac` to compile your program.

- `-a`: Output assembly (default=`false`).
- `-b`: Output binary (default=`false`).
- `-o`: Optimize the program (default=`true`).
- `-binary_out_dir`: Output location of JB files (default=`"./"`).
- `-assembly_out_dir`: Output location for JA files (default=`"./"`).

```shell
zinniac -a -b my_program.zn
```

## Running your program

Use `zinnia` to run your program either from source, assembly, or bytecode.

```shell
zinnia my_program.zn
# or
zinnia my_program.zna
# or
zinnia my_program.znb
```

## Examples

Check out the [examples](https://github.com/jeffmanzione/zinnia/tree/master/examples).
