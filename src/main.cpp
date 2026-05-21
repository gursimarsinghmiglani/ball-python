#include "backend/codegen.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic_analyzer/semantic_analyzer.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
static void print_usage() {
  std::cerr << "Usage: zornc <source.zn> [-o output] [--emit-ir] [--no-opt] "
               "[--emit-obj]\n";
  std::cerr << "\n";
  std::cerr << "Options:\n";
  std::cerr << "  -o <file>    Output executable name (default: name of source "
               "without extension)\n";
  std::cerr << "  --emit-ir    Print LLVM IR to stderr\n";
  std::cerr << "  --no-opt     Skip optimization passes\n";
  std::cerr << "  --emit-obj   Only emit object file, don't link\n";
}
int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage();
    return 1;
  }
  std::string source_file;
  std::string output_name;
  bool emit_ir = false;
  bool no_opt = false;
  bool emit_obj_only = false;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--emit-ir") {
      emit_ir = true;
    } else if (arg == "--no-opt") {
      no_opt = true;
    } else if (arg == "--emit-obj") {
      emit_obj_only = true;
    } else if (arg == "-o" && i + 1 < argc) {
      output_name = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      print_usage();
      return 0;
    } else if (arg[0] == '-') {
      std::cerr << "Unknown option: " << arg << "\n";
      print_usage();
      return 1;
    } else {
      source_file = arg;
    }
  }
  if (source_file.empty()) {
    std::cerr << "Error: no source file specified\n";
    print_usage();
    return 1;
  }
  namespace fs = std::filesystem;
  fs::path src_path(source_file);
  if (output_name.empty()) {
    output_name = src_path.stem().string();
  }
  std::ifstream file(source_file);
  if (!file.is_open()) {
    std::cerr << "Error: could not open file '" << source_file << "'\n";
    return 1;
  }
  std::stringstream buf;
  buf << file.rdbuf();
  std::string source = buf.str();
  auto lexemes = maximal_munch(source);
  Parser parser(lexemes);
  auto ast = parser.parse_program();
  SemanticAnalyzer analyzer(ast.get());
  CodeGen codegen;
  codegen.generate(ast.get());
  if (emit_ir) {
    codegen.dump_ir();
  }
  if (!no_opt) {
    codegen.optimize();
  }
  std::string obj_file = output_name + ".o";
  codegen.emit_object_code(obj_file);
  if (emit_obj_only) {
    std::cout << "Object file written to " << obj_file << "\n";
    return 0;
  }
  std::string runtime_path = ZORNRT_PATH;
  std::string link_cmd =
      "clang++ " + obj_file + " " + runtime_path + " -o " + output_name;
  int link_result = std::system(link_cmd.c_str());
  std::remove(obj_file.c_str());
  if (link_result != 0) {
    std::cerr << "Error: linking failed\n";
    return 1;
  }
  std::cout << "Compiled '" << source_file << "' -> './" << output_name
            << "'\n";
  return 0;
}
