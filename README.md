# Zorn Compiler

This repository contains the `zornc` compiler, which compiles Zorn source code (`.zn` files) into executable binaries using LLVM.

## Building the Compiler

### Prerequisites
- CMake (3.16 or higher)
- LLVM (The build system expects LLVM to be discoverable via `find_package`)
- Clang/Clang++ (for linking the final executable)
- A C++17 compatible compiler

### Build Instructions

To build the `zornc` compiler, run the following commands from the root of the project:

```bash
# Create a build directory
mkdir build
cd build

# Configure the project with CMake
cmake ..

# Build the compiler
make
```

After a successful build, the `zornc` compiler executable will be available in the `build` directory.

## Using the Compiler

The `zornc` compiler takes a Zorn source file as input and produces an executable binary.

### Basic Usage

```bash
./build/zornc <source.zn>
```
By default, this will produce an executable with the same name as the source file (without the `.zn` extension) in the current directory.

### Command Line Options

```bash
Usage: zornc <source.zn> [-o output] [--emit-ir] [--no-opt] [--emit-obj]

Options:
  -o <file>    Output executable name (default: name of source without extension)
  --emit-ir    Print LLVM IR to stderr
  --no-opt     Skip optimization passes
  --emit-obj   Only emit object file, don't link
```

### Examples

**Compile to a specific output executable:**
```bash
./build/zornc my_program.zn -o my_app
./my_app
```

**View the generated LLVM IR (Intermediate Representation):**
```bash
./build/zornc my_program.zn --emit-ir
```

**Compile without optimization passes:**
```bash
./build/zornc my_program.zn --no-opt
```

**Only emit the object file (do not invoke clang to link):**
```bash
./build/zornc my_program.zn --emit-obj
```
This will produce a `.o` object file.
