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
bazel build -c opt //runner:jlr
```

## Compiling your jeff-vm program
```
./bazel-bin/jlc -m my_program.jl
```

## Running your jeff-vm program
```
./bazel-bin/jlr my_program.jm
```
