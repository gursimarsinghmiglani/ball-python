cmake_minimum_required(VERSION 3.16)
project(ZornCompiler LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Build the Zorn runtime as a static library (linked into user programs)
add_library(zornrt STATIC src/backend/zorn_runtime.cpp)

# Build the compiler itself
add_executable(zornc
    src/main.cpp
    src/backend/codegen.cpp
)
target_include_directories(zornc PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# Tell the compiler binary where to find the runtime library at link time
target_compile_definitions(zornc PRIVATE
    ZORNRT_PATH="$<TARGET_FILE:zornrt>"
)
llvm_map_components_to_libnames(llvm_libs Core ExecutionEngine Interpreter Support native Target AsmPrinter AsmParser Passes Passes)
target_link_libraries(zornc PRIVATE ${llvm_libs})

# Make sure the runtime is built before the compiler
add_dependencies(zornc zornrt)
