# jeff-vm
A personal project to create a process virtual machine and language.

## Dependencies
bazel - https://bazel.build/

## Downloading and building this project 
```
# Clones this git repository.
git clone https://github.com/jeffreymanzione/jeff-vm.git

# Skip if you already have bazel installed.
sudo apt-get install bazel

# Bazel requires building within the workspace.
cd jeff-vm

# Builds the compiler.
bazel build -c opt //compile:jlc

# Builds the runner.
bazel build -c opt //run:jlr
```

## Compiling your jeff-vm program and outputs my_program.ja (assembly) and my_program.jb (bytecode).
```
./bazel-bin/compile/jlc -a -b my_program.jv
```

## Running your jeff-vm program
```
./bazel-bin/run/jlr my_program.jv
# or
./bazel-bin/run/jlr my_program.ja
# or
./bazel-bin/run/jlr my_program.jb
```
