
![Zinnia Logo](https://repository-images.githubusercontent.com/266423086/01b55ccc-4cd2-4d80-a260-24ffc896c6f4)

## A process virtual machine programming language that melds my favorite features of JavaScript and Java.

## Why use Zinnia?

I don't recommend it... yet. It's still an exploration.

It supports compilation of programs to the following outputs:

- **Executable** (.exe): A native executable file for the program that can be run without Zinnia.
- **Intermediate language** (.zna): An intermediate program representation of the source that is optimized.
- **Bytecode** (.znb): A bytecode representation of the program.
- **Bundle** (.znbundle): Archives of modules (and native libraries) that can be
  unpacked by Zinnia at runtime. The older .znseed extension is still accepted.

.zna, .znb, and .znbundle files can be run by the `zinnia` executable.

## Dependencies

- [bazel](https://bazel.build/) - Compile and building the entire application.
- [c-data-structures](https://github.com/jeffmanzione/c-data-structures) - Some useful data structures.
- [file-utils](https://github.com/jeffmanzione/file-utils) - Basic C file wrapper for reading files.
- [intern](https://github.com/jeffmanzione/intern) - String intern.
- [language-tools](https://github.com/jeffmanzione/language-tools) - Creating the lexer, parser, and semantic analyzer.
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
    //zinnia:zinnia-package  # Packager (formerly zinniap)
    //zinnia:zinnia-bundle   # Bundler (formerly zinnias)
)
# Build above binaries
bazel build -c opt "${args[@]}"
```

## Compili## Compiling your program to assembly and bytecode

Use `zinniac` to compile your program.

- `-a`, `--assembly`: Output assembly (.zna) (default=`false`).
- `-b`, `--bytecode`: Output bytecode (.znb) (default=`false`).
- `-m`, `--minimize`: Minimize the assembly output (default=`false`).
- `-o`, `--optimize`: Optimize the program (default=`true`).
- `--assembly_out_dir=DIR`: Output location for .zna files (default=`"./"`).
- `--binary_out_dir=DIR`: Output location for .znb files (default=`"./"`).

```shell
zinniac -a -b my_program.zn
```

## Running your program

Use `zinnia` to run your program either from source, assembly, bytecode, or a
bundle.

```shell
zinnia my_program.zn
# or
zinnia my_program.zna
# or
zinnia my_program.znb
```

Arguments after `--` are passed to the program and are available as `args`:

```shell
zinnia my_program.zn -- --name=value --flag --noother_flag
```

`zinnia` exits with the code given to `exit` (e.g., `exit 2`) in the main
module, or `1` if the program raises an uncaught error.

## Building programs with Bazel

`//zinnia:zinnia.bzl` provides rules for building Zinnia code with Bazel:

- `zinnia_library`: Compiles .zn sources to .zna and .znb files.
- `zinnia_binary`: Packages a program into a native executable.
- `zinnia_cc_library`: Statically links a C library into a program as a module.
- `zinnia_bundle`: Bundles modules into a .znbundle archive (formerly
  `zinnia_seed`).
- `zinnia_test`: Runs a program as a test, which passes if the program exits
  with code `0`.

See the [examples](https://github.com/jeffmanzione/zinnia/tree/master/examples)
for usage.

## Testing

```shell
bazel test //...
```

Tests are written with the `test` library, which raises an error (failing the
test) if any test fails:

```
import test

test.Tester().test(self)

@test.TestClass
class MyTest {
  @test.Test
  method test_add() {
    test.expect(1 + 1, 2)
  }
}
```

## Examples

Check out the [examples](https://github.com/jeffmanzione/zinnia/tree/master/examples).
