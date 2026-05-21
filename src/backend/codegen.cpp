#include "backend/codegen.hpp"
#include "parser/ast.hpp"
#include <iostream>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
CodeGen::CodeGen() {
  context = std::make_unique<llvm::LLVMContext>();
  builder = std::make_unique<llvm::IRBuilder<>>(*context);
  module = std::make_unique<llvm::Module>("zorn_module", *context);
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
}
llvm::Type *CodeGen::get_llvm_type(const Type &type) {
  switch (type.type_node) {
  case TypeNode::INT:
    return llvm::Type::getInt64Ty(*context);
  case TypeNode::FLOAT:
    return llvm::Type::getDoubleTy(*context);
  case TypeNode::BOOL:
    return llvm::Type::getInt1Ty(*context);
  case TypeNode::VOID:
    return llvm::Type::getVoidTy(*context);
  default:
    return llvm::PointerType::getUnqual(
        *context);
  }
}
llvm::AllocaInst *CodeGen::create_entry_block_alloca(
    llvm::Function *function, const std::string &var_name, llvm::Type *type) {
  llvm::IRBuilder<> tmp_builder(&function->getEntryBlock(),
                                function->getEntryBlock().begin());
  return tmp_builder.CreateAlloca(type, nullptr, var_name);
}
void CodeGen::generate(AST *root) { visit_program(root); }
void CodeGen::optimize() {
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;
  llvm::PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
  llvm::ModulePassManager MPM =
      PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
  MPM.run(*module, MAM);
}
void CodeGen::emit_object_code(const std::string &filename) {
  auto TargetTripleStr = llvm::sys::getDefaultTargetTriple();
  llvm::Triple TargetTriple(TargetTripleStr);
  module->setTargetTriple(TargetTriple);
  std::string Error;
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
  if (!Target) {
    std::cerr << Error;
    return;
  }
  auto CPU = "generic";
  auto Features = "";
  llvm::TargetOptions opt;
  auto RM = std::optional<llvm::Reloc::Model>();
  auto TargetMachine =
      Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
  module->setDataLayout(TargetMachine->createDataLayout());
  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
  if (EC) {
    std::cerr << "Could not open file: " << EC.message() << "\n";
    return;
  }
  llvm::legacy::PassManager pass;
  auto FileType = llvm::CodeGenFileType::ObjectFile;
  if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    std::cerr << "TargetMachine can't emit a file of this type\n";
    return;
  }
  pass.run(*module);
  dest.flush();
}
void CodeGen::dump_ir() { module->print(llvm::errs(), nullptr); }
llvm::Value *CodeGen::visit(AST *node) {
  if (!node)
    return nullptr;
  switch (node->node) {
  case Node::PROGRAM:
    return visit_program(node);
  case Node::VAR_DECL:
    return visit_var_decl(node);
  case Node::CONST_DECL:
    return visit_const_decl(node);
  case Node::FUNCTION_DECL:
    return visit_function_decl(node);
  case Node::BLOCK:
    return visit_block(node);
  case Node::IF_STMT:
    return visit_if_stmt(node);
  case Node::WHILE_STMT:
    return visit_while_stmt(node);
  case Node::RETURN_STMT:
    return visit_return_stmt(node);
  case Node::EXPR_STMT:
    return visit_expr_stmt(node);
  case Node::ASSIGN:
    return visit_assign_expr(node);
  case Node::ID:
  case Node::INT_LIT:
  case Node::FLOAT_LIT:
  case Node::BOOL:
  case Node::TENSOR_INIT:
  case Node::TENSOR_LIT:
  case Node::STRING_LIT:
    return visit_primary_expr(node);
  case Node::BINARY_OP:
    return visit_binary_expr(node);
  case Node::POSTFIX_OP:
    return visit_postfix_expr(node);
  case Node::UNARY_OP:
    return visit_unary_expr(node);
  case Node::PRINT_STMT:
    return visit_print_stmt(node);
  case Node::FOR_STMT:
    return visit_for_stmt(node);
  case Node::FN_CALL:
    return visit_postfix_expr(node);
  case Node::VAR_DEF:
    return visit_var_decl(node);
  default:
    return nullptr;
  }
}
llvm::Value *CodeGen::visit_program(AST *node) {
  llvm::FunctionType *initFT = llvm::FunctionType::get(llvm::Type::getVoidTy(*context), false);
  llvm::Function *initF = llvm::Function::Create(initFT, llvm::Function::InternalLinkage, "__zorn_global_init", module.get());
  llvm::BasicBlock *initBB = llvm::BasicBlock::Create(*context, "entry", initF);
  global_init_func = initF;
  for (const auto &child : node->children) {
    visit(child.get());
  }
  builder->SetInsertPoint(initBB);
  for (auto &inst : global_init_instructions) {
    inst();
  }
  builder->CreateRetVoid();
  global_init_func = nullptr;
  llvm::Type *i32Ty = llvm::Type::getInt32Ty(*context);
  llvm::StructType *ctorTy = llvm::StructType::get(i32Ty, llvm::PointerType::get(*context, 0), llvm::PointerType::getUnqual(*context));
  llvm::Constant *ctorEntry = llvm::ConstantStruct::get(ctorTy, {
    llvm::ConstantInt::get(i32Ty, 65535),
    initF,
    llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context))
  });
  llvm::ArrayType *ctorArrTy = llvm::ArrayType::get(ctorTy, 1);
  new llvm::GlobalVariable(*module, ctorArrTy, false, llvm::GlobalValue::AppendingLinkage,
      llvm::ConstantArray::get(ctorArrTy, {ctorEntry}), "llvm.global_ctors");
  return nullptr;
}
llvm::Value *CodeGen::visit_decl(AST *node) { return visit(node); }
llvm::Value *CodeGen::visit_var_decl(AST *node) {
  std::string id = std::get<std::string>(node->children[0]->v);
  if (!current_function) {
    llvm::Type *type = get_llvm_type(node->type);
    llvm::Constant *init = nullptr;
    if (type->isIntegerTy(64)) {
      init = llvm::ConstantInt::get(type, 0);
    } else if (type->isDoubleTy()) {
      init = llvm::ConstantFP::get(type, 0.0);
    } else if (type->isIntegerTy(1)) {
      init = llvm::ConstantInt::get(type, 0);
    } else {
      init = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
    }
    auto *gv = new llvm::GlobalVariable(*module, type, false,
        llvm::GlobalValue::InternalLinkage, init, id);
    SymbolInfo sym_info(node->type, false);
    sym_info.llvm_value = gv;
    st.declare(id, sym_info);
    bool has_init = node->children.size() > 2 || (node->children.size() > 1 && node->children[1]->node != Node::TYPE);
    if (has_init) {
      AST *init_expr = node->children.size() > 2 ? node->children[2].get() : node->children[1].get();
      global_init_instructions.push_back([this, init_expr, gv]() {
        llvm::Function *saved = current_function;
        current_function = global_init_func;
        llvm::Value *val = visit(init_expr);
        builder->CreateStore(val, gv);
        current_function = saved;
      });
    }
    return gv;
  }
  llvm::Value *init_val = nullptr;
  if (node->children.size() > 2) {
    init_val = visit(node->children[2].get());
  } else if (node->children[1]->node != Node::TYPE) {
    init_val = visit(node->children[1].get());
  }
  llvm::Type *type = get_llvm_type(node->type);
  llvm::AllocaInst *alloca = create_entry_block_alloca(current_function, id, type);
  if (init_val) {
    builder->CreateStore(init_val, alloca);
  }
  SymbolInfo sym_info(node->type, false);
  sym_info.llvm_value = alloca;
  st.declare(id, sym_info);
  return alloca;
}
llvm::Value *CodeGen::visit_const_decl(AST *node) {
  std::string id = std::get<std::string>(node->children[0]->v);
  if (!current_function) {
    llvm::Type *type = get_llvm_type(node->type);
    llvm::Constant *init = nullptr;
    if (type->isIntegerTy(64)) {
      init = llvm::ConstantInt::get(type, 0);
    } else if (type->isDoubleTy()) {
      init = llvm::ConstantFP::get(type, 0.0);
    } else if (type->isIntegerTy(1)) {
      init = llvm::ConstantInt::get(type, 0);
    } else {
      init = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
    }
    auto *gv = new llvm::GlobalVariable(*module, type, false,
        llvm::GlobalValue::InternalLinkage, init, id);
    SymbolInfo sym_info(node->type, true);
    sym_info.llvm_value = gv;
    st.declare(id, sym_info);
    AST *init_expr = node->children[1]->node == Node::TYPE ? node->children[2].get() : node->children[1].get();
    global_init_instructions.push_back([this, init_expr, gv]() {
      llvm::Function *saved = current_function;
      current_function = global_init_func;
      llvm::Value *val = visit(init_expr);
      builder->CreateStore(val, gv);
      current_function = saved;
    });
    return gv;
  }
  llvm::Value *init_val =
      visit(node->children[1]->node == Node::TYPE ? node->children[2].get()
                                                  : node->children[1].get());
  llvm::Type *type = get_llvm_type(node->type);
  llvm::AllocaInst *alloca = create_entry_block_alloca(current_function, id, type);
  builder->CreateStore(init_val, alloca);
  SymbolInfo sym_info(node->type, true);
  sym_info.llvm_value = alloca;
  st.declare(id, sym_info);
  return alloca;
}
llvm::Value *CodeGen::visit_function_decl(AST *node) {
  std::string fn_name = std::get<std::string>(node->children[0]->v);
  std::vector<llvm::Type *> param_types;
  std::vector<std::string> param_names;
  Type ret_type = Type::from_type_node(TypeNode::VOID);
  for (int i = 1; i < node->children.size() - 1; i++) {
    if (node->children[i]->node == Node::PARAM) {
      param_types.push_back(
          get_llvm_type(node->children[i]->children[1]->type));
      param_names.push_back(
          std::get<std::string>(node->children[i]->children[0]->v));
    } else {
      ret_type = node->children[i]->type;
    }
  }
  llvm::Type *llvm_ret_type = get_llvm_type(ret_type);
  llvm::FunctionType *FT =
      llvm::FunctionType::get(llvm_ret_type, param_types, false);
  llvm::Function *F = llvm::Function::Create(
      FT, llvm::Function::ExternalLinkage, fn_name, module.get());
  unsigned idx = 0;
  for (auto &arg : F->args()) {
    arg.setName(param_names[idx++]);
  }
  SymbolInfo sym_info(ret_type, std::vector<Type>());
  sym_info.is_function = true;
  sym_info.llvm_value = F;
  st.declare(fn_name, sym_info);
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(*context, "entry", F);
  builder->SetInsertPoint(BB);
  current_function = F;
  st.enter_scope();
  idx = 0;
  for (auto &arg : F->args()) {
    llvm::AllocaInst *alloca =
        create_entry_block_alloca(F, param_names[idx], arg.getType());
    builder->CreateStore(&arg, alloca);
    SymbolInfo param_info(node->children[idx + 1]->children[1]->type, false);
    param_info.llvm_value = alloca;
    st.declare(param_names[idx], param_info);
    idx++;
  }
  visit_block(node->children.back().get());
  st.exit_scope();
  if (ret_type.type_node == TypeNode::VOID &&
      !builder->GetInsertBlock()->getTerminator()) {
    builder->CreateRetVoid();
  }
  llvm::verifyFunction(*F);
  current_function = nullptr;
  return F;
}
llvm::Value *CodeGen::visit_block(AST *node) {
  for (const auto &child : node->children) {
    if (child->node == Node::BLOCK) {
      st.enter_scope();
      visit(child.get());
      st.exit_scope();
    } else {
      visit(child.get());
    }
  }
  return nullptr;
}
llvm::Value *CodeGen::visit_if_stmt(AST *node) {
  llvm::Value *cond = visit(node->children[0].get());
  if (!cond) return nullptr;
  llvm::Function *the_function = builder->GetInsertBlock()->getParent();
  llvm::BasicBlock *then_bb =
      llvm::BasicBlock::Create(*context, "then", the_function);
  llvm::BasicBlock *else_bb = llvm::BasicBlock::Create(*context, "else");
  llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(*context, "ifcont");
  bool has_else = node->children.size() > 2;
  builder->CreateCondBr(cond, then_bb, has_else ? else_bb : merge_bb);
  builder->SetInsertPoint(then_bb);
  st.enter_scope();
  visit(node->children[1].get());
  st.exit_scope();
  if (!builder->GetInsertBlock()->getTerminator()) {
    builder->CreateBr(merge_bb);
  }
  if (has_else) {
    the_function->insert(the_function->end(), else_bb);
    builder->SetInsertPoint(else_bb);
    st.enter_scope();
    visit(node->children[2].get());
    st.exit_scope();
    if (!builder->GetInsertBlock()->getTerminator()) {
      builder->CreateBr(merge_bb);
    }
  }
  the_function->insert(the_function->end(), merge_bb);
  builder->SetInsertPoint(merge_bb);
  return nullptr;
}
llvm::Value *CodeGen::visit_while_stmt(AST *node) {
  llvm::Function *the_function = builder->GetInsertBlock()->getParent();
  llvm::BasicBlock *cond_bb =
      llvm::BasicBlock::Create(*context, "cond", the_function);
  llvm::BasicBlock *loop_bb =
      llvm::BasicBlock::Create(*context, "loop", the_function);
  llvm::BasicBlock *after_bb =
      llvm::BasicBlock::Create(*context, "afterloop", the_function);
  builder->CreateBr(cond_bb);
  builder->SetInsertPoint(cond_bb);
  llvm::Value *cond = visit(node->children[0].get());
  builder->CreateCondBr(cond, loop_bb, after_bb);
  builder->SetInsertPoint(loop_bb);
  st.enter_scope();
  visit(node->children[1].get());
  st.exit_scope();
  if (!builder->GetInsertBlock()->getTerminator()) {
    builder->CreateBr(cond_bb);
  }
  builder->SetInsertPoint(after_bb);
  return nullptr;
}
llvm::Value *CodeGen::visit_return_stmt(AST *node) {
  if (node->children.empty()) {
    return builder->CreateRetVoid();
  } else {
    llvm::Value *ret_val = visit(node->children[0].get());
    if (!ret_val) return builder->CreateRetVoid();
    return builder->CreateRet(ret_val);
  }
}
llvm::Value *CodeGen::visit_expr_stmt(AST *node) {
  return visit(node->children[0].get());
}
llvm::Value *CodeGen::visit_assign_expr(AST *node) {
  llvm::Value *val = visit(node->children[1].get());
  if (node->children[0]->node == Node::ID) {
    std::string name = std::get<std::string>(node->children[0]->v);
    SymbolInfo *sym = st.lookup(name);
    if (sym && sym->llvm_value) {
      builder->CreateStore(val, sym->llvm_value);
      return val;
    }
  } else if (node->children[0]->node == Node::POSTFIX_OP) {
    auto post_node = node->children[0].get();
    if (std::holds_alternative<PostfixOpNode>(post_node->v) &&
        std::get<PostfixOpNode>(post_node->v) == PostfixOpNode::INDEX) {
      llvm::Value *tensor_val = visit(post_node->children[0].get());
      llvm::Function *F = module->getFunction("zorn_tensor_set");
      if (!F) {
        llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::Type::getVoidTy(*context),
                                    {llvm::PointerType::getUnqual(*context),
                                     llvm::Type::getInt64Ty(*context),
                                     llvm::PointerType::getUnqual(*context),
                                     llvm::Type::getDoubleTy(*context)},
                                    false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                   "zorn_tensor_set", module.get());
      }
      int64_t num_indices = post_node->children.size() - 1;
      llvm::Value *num_idx_val =
          llvm::ConstantInt::get(*context, llvm::APInt(64, num_indices, true));
      llvm::AllocaInst *indices_array =
          builder->CreateAlloca(llvm::Type::getInt64Ty(*context), num_idx_val);
      for (size_t i = 1; i <= num_indices; i++) {
        llvm::Value *idx_val = visit(post_node->children[i].get());
        llvm::Value *offset =
            llvm::ConstantInt::get(*context, llvm::APInt(64, i - 1, true));
        llvm::Value *ptr = builder->CreateGEP(llvm::Type::getInt64Ty(*context),
                                              indices_array, {offset});
        builder->CreateStore(idx_val, ptr);
      }
      llvm::Value *double_val = val;
      if (val->getType()->isIntegerTy())
        double_val = builder->CreateSIToFP(
            val, llvm::Type::getDoubleTy(*context), "casttodouble");
      builder->CreateCall(F,
                          {tensor_val, num_idx_val, indices_array, double_val});
      return val;
    }
  }
  return nullptr;
}
llvm::Value *CodeGen::visit_primary_expr(AST *node) {
  switch (node->node) {
  case Node::INT_LIT:
    return llvm::ConstantInt::get(
        *context, llvm::APInt(64, std::get<int64_t>(node->v), true));
  case Node::FLOAT_LIT:
    return llvm::ConstantFP::get(*context,
                                 llvm::APFloat(std::get<double>(node->v)));
  case Node::BOOL:
    return llvm::ConstantInt::get(
        *context, llvm::APInt(1, std::get<bool>(node->v) ? 1 : 0));
  case Node::STRING_LIT: {
    std::string str_val = std::get<std::string>(node->v);
    llvm::GlobalVariable *str_global = builder->CreateGlobalString(str_val, "str");
    return builder->CreateConstGEP2_32(str_global->getValueType(), str_global, 0, 0, "strptr");
  }
  case Node::ID: {
    std::string name = std::get<std::string>(node->v);
    SymbolInfo *sym = st.lookup(name);
    if (sym && sym->llvm_value) {
      if (sym->is_function) {
        return sym->llvm_value;
      }
      return builder->CreateLoad(get_llvm_type(sym->type), sym->llvm_value,
                                 name);
    }
    return nullptr;
  }
  case Node::TENSOR_INIT: {
    llvm::Function *F = module->getFunction("zorn_tensor_create");
    if (!F) {
      llvm::FunctionType *FT =
          llvm::FunctionType::get(llvm::PointerType::getUnqual(*context),
                                  {llvm::Type::getInt64Ty(*context),
                                   llvm::PointerType::getUnqual(*context)},
                                  false);
      F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                 "zorn_tensor_create", module.get());
    }
    int64_t ndim = node->type.sizes.size();
    llvm::Value *ndim_val =
        llvm::ConstantInt::get(*context, llvm::APInt(64, ndim, true));
    llvm::AllocaInst *sizes_array =
        builder->CreateAlloca(llvm::Type::getInt64Ty(*context), ndim_val);
    for (size_t i = 0; i < (size_t)ndim; i++) {
      llvm::Value *idx =
          llvm::ConstantInt::get(*context, llvm::APInt(64, i, true));
      llvm::Value *ptr = builder->CreateGEP(llvm::Type::getInt64Ty(*context),
                                            sizes_array, {idx});
      llvm::Value *size_val = llvm::ConstantInt::get(
          *context, llvm::APInt(64, node->type.sizes[i], true));
      builder->CreateStore(size_val, ptr);
    }
    llvm::Value *tensor = builder->CreateCall(F, {ndim_val, sizes_array});

    llvm::Value *default_val = visit_expr(node->children[1].get());
    if (default_val->getType()->isIntegerTy(1)) {
        default_val = builder->CreateUIToFP(default_val, llvm::Type::getDoubleTy(*context));
    } else if (default_val->getType()->isIntegerTy(64)) {
        default_val = builder->CreateSIToFP(default_val, llvm::Type::getDoubleTy(*context));
    }

    llvm::Function *fillF = module->getFunction("zorn_tensor_fill");
    if (!fillF) {
      llvm::FunctionType *fillFT = llvm::FunctionType::get(
          llvm::Type::getVoidTy(*context),
          {llvm::PointerType::getUnqual(*context), llvm::Type::getDoubleTy(*context)}, false);
      fillF = llvm::Function::Create(fillFT, llvm::Function::ExternalLinkage, "zorn_tensor_fill", module.get());
    }
    builder->CreateCall(fillF, {tensor, default_val});
    return tensor;
  }
  case Node::TENSOR_LIT: {
    if (node->children.size() > 0) {
      llvm::Function *F = module->getFunction("zorn_tensor_create");
      if (!F) {
        llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::PointerType::getUnqual(*context),
                                    {llvm::Type::getInt64Ty(*context),
                                     llvm::PointerType::getUnqual(*context)},
                                    false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                   "zorn_tensor_create", module.get());
      }
      int64_t ndim = node->type.sizes.size();
      llvm::Value *ndim_val =
          llvm::ConstantInt::get(*context, llvm::APInt(64, ndim, true));
      llvm::AllocaInst *sizes_array =
          builder->CreateAlloca(llvm::Type::getInt64Ty(*context), ndim_val);
      for (size_t i = 0; i < (size_t)ndim; i++) {
        llvm::Value *idx =
            llvm::ConstantInt::get(*context, llvm::APInt(64, i, true));
        llvm::Value *ptr = builder->CreateGEP(llvm::Type::getInt64Ty(*context),
                                              sizes_array, {idx});
        llvm::Value *size_val = llvm::ConstantInt::get(
            *context, llvm::APInt(64, node->type.sizes[i], true));
        builder->CreateStore(size_val, ptr);
      }
      llvm::Value *tensor = builder->CreateCall(F, {ndim_val, sizes_array});
      llvm::Function *setF = module->getFunction("zorn_tensor_set");
      if (!setF) {
        llvm::FunctionType *setFT = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context),
            {llvm::PointerType::getUnqual(*context), llvm::Type::getInt64Ty(*context),
             llvm::PointerType::getUnqual(*context), llvm::Type::getDoubleTy(*context)}, false);
        setF = llvm::Function::Create(setFT, llvm::Function::ExternalLinkage, "zorn_tensor_set", module.get());
      }
      if (ndim == 1) {
        llvm::Value *one_idx = llvm::ConstantInt::get(*context, llvm::APInt(64, 1, true));
        llvm::AllocaInst *idx_slot = builder->CreateAlloca(llvm::Type::getInt64Ty(*context), one_idx);
        for (size_t i = 0; i < node->children.size(); i++) {
          llvm::Value *elem = visit(node->children[i].get());
          llvm::Value *elem_dbl = elem;
          if (elem->getType()->isIntegerTy())
            elem_dbl = builder->CreateSIToFP(elem, llvm::Type::getDoubleTy(*context), "litcvt");
          llvm::Value *idx_val = llvm::ConstantInt::get(*context, llvm::APInt(64, i, true));
          builder->CreateStore(idx_val, idx_slot);
          builder->CreateCall(setF, {tensor, one_idx, idx_slot, elem_dbl});
        }
      } else {
        llvm::Function *getF = module->getFunction("zorn_tensor_get");
        if (!getF) {
          llvm::FunctionType *getFT = llvm::FunctionType::get(
              llvm::Type::getDoubleTy(*context),
              {llvm::PointerType::getUnqual(*context), llvm::Type::getInt64Ty(*context),
               llvm::PointerType::getUnqual(*context)}, false);
          getF = llvm::Function::Create(getFT, llvm::Function::ExternalLinkage, "zorn_tensor_get", module.get());
        }
        llvm::Value *ndim_set = llvm::ConstantInt::get(*context, llvm::APInt(64, ndim, true));
        llvm::AllocaInst *idx_arr = builder->CreateAlloca(llvm::Type::getInt64Ty(*context), ndim_set);
        llvm::Value *one_sub = llvm::ConstantInt::get(*context, llvm::APInt(64, 1, true));
        llvm::AllocaInst *sub_idx = builder->CreateAlloca(llvm::Type::getInt64Ty(*context), one_sub);
        for (size_t i = 0; i < node->children.size(); i++) {
          llvm::Value *row_idx = llvm::ConstantInt::get(*context, llvm::APInt(64, i, true));
          llvm::Value *ptr0 = builder->CreateGEP(llvm::Type::getInt64Ty(*context), idx_arr,
              {llvm::ConstantInt::get(*context, llvm::APInt(64, 0, true))});
          builder->CreateStore(row_idx, ptr0);
          if (node->children[i]->node == Node::TENSOR_LIT) {
            llvm::Value *sub_tensor = visit(node->children[i].get());
            for (size_t j = 0; j < node->children[i]->children.size(); j++) {
              llvm::Value *col_idx = llvm::ConstantInt::get(*context, llvm::APInt(64, j, true));
              builder->CreateStore(col_idx, sub_idx);
              llvm::Value *val = builder->CreateCall(getF, {sub_tensor, one_sub, sub_idx}, "subval");
              llvm::Value *ptr1 = builder->CreateGEP(llvm::Type::getInt64Ty(*context), idx_arr,
                  {llvm::ConstantInt::get(*context, llvm::APInt(64, 1, true))});
              builder->CreateStore(col_idx, ptr1);
              builder->CreateCall(setF, {tensor, ndim_set, idx_arr, val});
            }
          } else {
            llvm::Value *elem = visit(node->children[i].get());
            llvm::Value *elem_dbl = elem;
            if (elem->getType()->isIntegerTy())
              elem_dbl = builder->CreateSIToFP(elem, llvm::Type::getDoubleTy(*context), "litcvt");
            builder->CreateCall(setF, {tensor, one_sub, idx_arr, elem_dbl});
          }
        }
      }
      return tensor;
    }
    return nullptr;
  }
  default:
    return nullptr;
  }
}
llvm::Value *CodeGen::visit_expr(AST *node) {
  if (node->node == Node::ASSIGN)
    return visit_assign_expr(node);
  if (node->node == Node::BINARY_OP)
    return visit_binary_expr(node);
  if (node->node == Node::POSTFIX_OP)
    return visit_postfix_expr(node);
  if (node->node == Node::UNARY_OP)
    return visit_unary_expr(node);
  if (node->node == Node::FN_CALL)
    return visit_postfix_expr(node);
  return visit_primary_expr(node);
}
llvm::Value *CodeGen::visit_binary_expr(AST *node) {
  llvm::Value *l = visit(node->children[0].get());
  llvm::Value *r = visit(node->children[1].get());
  if (!l || !r)
    return nullptr;
  bool is_tensor = node->children[0]->type.type_node == TypeNode::MATRIX ||
                   node->children[0]->type.type_node == TypeNode::VECTOR ||
                   node->children[0]->type.type_node == TypeNode::TENSOR;
  bool result_is_float = node->type.type_node == TypeNode::FLOAT;
  auto promote = [&]() {
    if (result_is_float) {
      if (l->getType()->isIntegerTy(64))
        l = builder->CreateSIToFP(l, llvm::Type::getDoubleTy(*context), "promo_l");
      if (r->getType()->isIntegerTy(64))
        r = builder->CreateSIToFP(r, llvm::Type::getDoubleTy(*context), "promo_r");
    }
  };
  if (std::holds_alternative<AdditiveOpNode>(node->v)) {
    auto op = std::get<AdditiveOpNode>(node->v);
    if (is_tensor) {
      llvm::Function *F = module->getFunction(
          op == AdditiveOpNode::PLUS ? "zorn_tensor_add" : "zorn_tensor_sub");
      if (!F) {
        llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::PointerType::getUnqual(*context),
                                    {llvm::PointerType::getUnqual(*context),
                                     llvm::PointerType::getUnqual(*context)},
                                    false);
        F = llvm::Function::Create(
            FT, llvm::Function::ExternalLinkage,
            op == AdditiveOpNode::PLUS ? "zorn_tensor_add" : "zorn_tensor_sub",
            module.get());
      }
      return builder->CreateCall(F, {l, r});
    } else {
      promote();
      if (op == AdditiveOpNode::PLUS)
        return result_is_float ? builder->CreateFAdd(l, r, "faddtmp")
                               : builder->CreateAdd(l, r, "addtmp");
      if (op == AdditiveOpNode::MINUS)
        return result_is_float ? builder->CreateFSub(l, r, "fsubtmp")
                               : builder->CreateSub(l, r, "subtmp");
    }
  } else if (std::holds_alternative<MultiplicativeOpNode>(node->v)) {
    auto op = std::get<MultiplicativeOpNode>(node->v);
    if (op == MultiplicativeOpNode::MAT_MUL) {
      llvm::Function *F = module->getFunction("zorn_mat_mul");
      if (!F) {
        llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::PointerType::getUnqual(*context),
                                    {llvm::PointerType::getUnqual(*context),
                                     llvm::PointerType::getUnqual(*context)},
                                    false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                   "zorn_mat_mul", module.get());
      }
      return builder->CreateCall(F, {l, r});
    } else if (is_tensor) {
      const char* fn_name = "zorn_tensor_mul";
      if (op == MultiplicativeOpNode::DIV) fn_name = "zorn_tensor_div";
      else if (op == MultiplicativeOpNode::MOD) fn_name = "zorn_tensor_mod";
      llvm::Function *F = module->getFunction(fn_name);
      if (!F) {
        llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::PointerType::getUnqual(*context),
                                    {llvm::PointerType::getUnqual(*context),
                                     llvm::PointerType::getUnqual(*context)},
                                    false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                   fn_name,
                                   module.get());
      }
      return builder->CreateCall(F, {l, r});
    } else {
      promote();
      if (op == MultiplicativeOpNode::MUL)
        return result_is_float ? builder->CreateFMul(l, r, "fmultmp")
                               : builder->CreateMul(l, r, "multmp");
      if (op == MultiplicativeOpNode::DIV)
        return result_is_float ? builder->CreateFDiv(l, r, "fdivtmp")
                               : builder->CreateSDiv(l, r, "divtmp");
      if (op == MultiplicativeOpNode::MOD)
        return result_is_float ? builder->CreateFRem(l, r, "fmodtmp")
                               : builder->CreateSRem(l, r, "modtmp");
    }
  } else if (std::holds_alternative<LogicalOpNode>(node->v)) {
    auto op = std::get<LogicalOpNode>(node->v);
    if (op == LogicalOpNode::AND)
      return builder->CreateLogicalAnd(l, r, "andtmp");
    if (op == LogicalOpNode::OR)
      return builder->CreateLogicalOr(l, r, "ortmp");
  } else if (std::holds_alternative<BitwiseOpNode>(node->v)) {
    auto op = std::get<BitwiseOpNode>(node->v);
    if (op == BitwiseOpNode::BITWISE_AND)
      return builder->CreateAnd(l, r, "bitandtmp");
    if (op == BitwiseOpNode::BITWISE_OR)
      return builder->CreateOr(l, r, "bitorptmp");
    if (op == BitwiseOpNode::BITWISE_XOR)
      return builder->CreateXor(l, r, "bitxortmp");
  } else if (std::holds_alternative<EqualityOpNode>(node->v)) {
    auto op = std::get<EqualityOpNode>(node->v);
    bool either_float = node->children[0]->type.type_node == TypeNode::FLOAT ||
                        node->children[1]->type.type_node == TypeNode::FLOAT;
    if (either_float) {
      if (l->getType()->isIntegerTy(64))
        l = builder->CreateSIToFP(l, llvm::Type::getDoubleTy(*context), "promo_l");
      if (r->getType()->isIntegerTy(64))
        r = builder->CreateSIToFP(r, llvm::Type::getDoubleTy(*context), "promo_r");
    }
    if (op == EqualityOpNode::EQ)
      return either_float ? builder->CreateFCmpOEQ(l, r, "eqtmp")
                          : builder->CreateICmpEQ(l, r, "eqtmp");
    if (op == EqualityOpNode::NEQ)
      return either_float ? builder->CreateFCmpONE(l, r, "netmp")
                          : builder->CreateICmpNE(l, r, "netmp");
  } else if (std::holds_alternative<RelationalOpNode>(node->v)) {
    auto op = std::get<RelationalOpNode>(node->v);
    bool either_float = node->children[0]->type.type_node == TypeNode::FLOAT ||
                        node->children[1]->type.type_node == TypeNode::FLOAT;
    if (either_float) {
      if (l->getType()->isIntegerTy(64))
        l = builder->CreateSIToFP(l, llvm::Type::getDoubleTy(*context), "promo_l");
      if (r->getType()->isIntegerTy(64))
        r = builder->CreateSIToFP(r, llvm::Type::getDoubleTy(*context), "promo_r");
    }
    if (op == RelationalOpNode::LESS)
      return either_float ? builder->CreateFCmpOLT(l, r, "lttmp")
                          : builder->CreateICmpSLT(l, r, "lttmp");
    if (op == RelationalOpNode::LEQ)
      return either_float ? builder->CreateFCmpOLE(l, r, "letmp")
                          : builder->CreateICmpSLE(l, r, "letmp");
    if (op == RelationalOpNode::GREATER)
      return either_float ? builder->CreateFCmpOGT(l, r, "gttmp")
                          : builder->CreateICmpSGT(l, r, "gttmp");
    if (op == RelationalOpNode::GEQ)
      return either_float ? builder->CreateFCmpOGE(l, r, "getmp")
                          : builder->CreateICmpSGE(l, r, "getmp");
  }
  return nullptr;
}
llvm::Value *CodeGen::visit_postfix_expr(AST *node) {
  if (!std::holds_alternative<PostfixOpNode>(node->v))
    return nullptr;
  auto op = std::get<PostfixOpNode>(node->v);
  if (op == PostfixOpNode::ARGUMENT) {
    if (std::holds_alternative<std::string>(node->children[0]->v)) {
      std::string name = std::get<std::string>(node->children[0]->v);
      SymbolInfo *sym = st.lookup(name);
      if (sym && sym->is_function) {
        llvm::Function *calleeF = llvm::cast<llvm::Function>(sym->llvm_value);
        std::vector<llvm::Value *> args;
        for (size_t i = 1; i < node->children.size(); i++) {
          args.push_back(visit(node->children[i].get()));
        }
        if (calleeF->getReturnType()->isVoidTy())
          return builder->CreateCall(calleeF, args);
        return builder->CreateCall(calleeF, args, "calltmp");
      }
    }
  } else if (op == PostfixOpNode::INDEX) {
    llvm::Value *tensor_val = visit(node->children[0].get());
    llvm::Function *F = module->getFunction("zorn_tensor_get");
    if (!F) {
      llvm::FunctionType *FT =
          llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                                  {llvm::PointerType::getUnqual(*context),
                                   llvm::Type::getInt64Ty(*context),
                                   llvm::PointerType::getUnqual(*context)},
                                  false);
      F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                 "zorn_tensor_get", module.get());
    }
    int64_t num_indices = node->children.size() - 1;
    llvm::Value *num_idx_val =
        llvm::ConstantInt::get(*context, llvm::APInt(64, num_indices, true));
    llvm::AllocaInst *indices_array =
        builder->CreateAlloca(llvm::Type::getInt64Ty(*context), num_idx_val);
    for (size_t i = 1; i <= (size_t)num_indices; i++) {
      llvm::Value *idx_val = visit(node->children[i].get());
      llvm::Value *offset =
          llvm::ConstantInt::get(*context, llvm::APInt(64, i - 1, true));
      llvm::Value *ptr = builder->CreateGEP(llvm::Type::getInt64Ty(*context),
                                            indices_array, {offset});
      builder->CreateStore(idx_val, ptr);
    }
    llvm::Value *result = builder->CreateCall(F, {tensor_val, num_idx_val, indices_array}, "gettmp");
    if (node->type.type_node == TypeNode::INT) {
      return builder->CreateFPToSI(result, llvm::Type::getInt64Ty(*context), "idxcvt");
    } else if (node->type.type_node == TypeNode::BOOL) {
      return builder->CreateFCmpUNE(result, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "boolcvt");
    }
    return result;
  } else {
    llvm::Value *operand = visit(node->children[0].get());
    if (op == PostfixOpNode::TRANSPOSE) {
      llvm::Function *F = module->getFunction("zorn_transpose");
      if (!F) {
        llvm::FunctionType *FT = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context),
            {llvm::PointerType::getUnqual(*context)}, false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                   "zorn_transpose", module.get());
      }
      return builder->CreateCall(F, {operand});
    } else if (op == PostfixOpNode::INVERSE) {
      llvm::Function *F = module->getFunction("zorn_inverse");
      if (!F) {
        llvm::FunctionType *FT = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context),
            {llvm::PointerType::getUnqual(*context)}, false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                   "zorn_inverse", module.get());
      }
      return builder->CreateCall(F, {operand});
    } else if (op == PostfixOpNode::DOT) {
      llvm::Function *F = module->getFunction("zorn_tensor_shape");
      if (!F) {
        llvm::FunctionType *FT = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context),
            {llvm::PointerType::getUnqual(*context)}, false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                   "zorn_tensor_shape", module.get());
      }
      return builder->CreateCall(F, {operand});
    }
  }
  return nullptr;
}
llvm::Value *CodeGen::visit_print_stmt(AST *node) {
  llvm::Value *val = visit(node->children[0].get());
  auto type_node = node->children[0]->type.type_node;
  bool is_println = std::holds_alternative<PrintNode>(node->v) &&
                    std::get<PrintNode>(node->v) == PrintNode::PRINTLN;
  std::string suffix = is_println ? "" : "_nn";
  if (type_node == TypeNode::MATRIX || type_node == TypeNode::VECTOR ||
      type_node == TypeNode::TENSOR) {
    std::string fname = "zorn_print_tensor" + suffix;
    llvm::Function *F = module->getFunction(fname);
    if (!F) {
      llvm::FunctionType *FT = llvm::FunctionType::get(
          llvm::Type::getVoidTy(*context),
          {llvm::PointerType::getUnqual(*context)}, false);
      F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fname,
                                 module.get());
    }
    builder->CreateCall(F, {val});
  } else if (type_node == TypeNode::FLOAT) {
    std::string fname = "zorn_print_float" + suffix;
    llvm::Function *F = module->getFunction(fname);
    if (!F) {
      llvm::FunctionType *FT =
          llvm::FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {llvm::Type::getDoubleTy(*context)}, false);
      F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fname,
                                 module.get());
    }
    builder->CreateCall(F, {val});
  } else if (type_node == TypeNode::BOOL) {
    std::string fname = "zorn_print_bool" + suffix;
    llvm::Function *F = module->getFunction(fname);
    if (!F) {
      llvm::FunctionType *FT =
          llvm::FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {llvm::Type::getInt1Ty(*context)}, false);
      F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fname,
                                 module.get());
    }
    builder->CreateCall(F, {val});
  } else if (type_node == TypeNode::STRING) {
    std::string fname = "zorn_print_string" + suffix;
    llvm::Function *F = module->getFunction(fname);
    if (!F) {
      llvm::FunctionType *FT =
          llvm::FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {llvm::PointerType::getUnqual(*context)}, false);
      F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fname,
                                 module.get());
    }
    builder->CreateCall(F, {val});
  } else {
    std::string fname = "zorn_print_int" + suffix;
    llvm::Function *F = module->getFunction(fname);
    if (!F) {
      llvm::FunctionType *FT =
          llvm::FunctionType::get(llvm::Type::getVoidTy(*context),
                                  {llvm::Type::getInt64Ty(*context)}, false);
      F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fname,
                                 module.get());
    }
    builder->CreateCall(F, {val});
  }
  return nullptr;
}
llvm::Value *CodeGen::visit_unary_expr(AST *node) {
  if (std::holds_alternative<UnaryOpNode>(node->v)) {
    auto op = std::get<UnaryOpNode>(node->v);
    llvm::Value *operand = visit(node->children[0].get());
    if (!operand)
      return nullptr;
    bool is_float = node->children[0]->type.type_node == TypeNode::FLOAT;
    if (op == UnaryOpNode::MINUS)
      return is_float ? builder->CreateFNeg(operand, "negtmp")
                      : builder->CreateNeg(operand, "negtmp");
    if (op == UnaryOpNode::NOT)
      return builder->CreateXor(operand, llvm::ConstantInt::get(operand->getType(), 1), "nottmp");
    if (op == UnaryOpNode::BITWISE_NOT)
      return builder->CreateNot(operand, "bitnotmp");
    if (op == UnaryOpNode::PLUS)
      return operand;
  }
  return nullptr;
}
llvm::Value *CodeGen::visit_for_stmt(AST *node) {
  std::string var_name = std::get<std::string>(node->children[0]->v);
  AST *iter_node = node->children[1].get();
  llvm::Function *the_function = builder->GetInsertBlock()->getParent();
  if (iter_node->node == Node::RANGE) {
    llvm::Value *start_val = visit(iter_node->children[0].get());
    llvm::Value *end_val = visit(iter_node->children[1].get());
    llvm::Value *step_val = iter_node->children.size() > 2
        ? visit(iter_node->children[2].get())
        : llvm::ConstantInt::get(*context, llvm::APInt(64, 1, true));
    llvm::AllocaInst *alloca = create_entry_block_alloca(
        the_function, var_name, llvm::Type::getInt64Ty(*context));
    builder->CreateStore(start_val, alloca);
    llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(*context, "forcond", the_function);
    llvm::BasicBlock *loop_bb = llvm::BasicBlock::Create(*context, "forloop", the_function);
    llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(*context, "forafter", the_function);
    builder->CreateBr(cond_bb);
    builder->SetInsertPoint(cond_bb);
    llvm::Value *curr_val = builder->CreateLoad(llvm::Type::getInt64Ty(*context), alloca, var_name);
    builder->CreateCondBr(builder->CreateICmpSLT(curr_val, end_val, "forcond"), loop_bb, after_bb);
    builder->SetInsertPoint(loop_bb);
    st.enter_scope();
    SymbolInfo sym_info(Type::from_type_node(TypeNode::INT), false);
    sym_info.llvm_value = alloca;
    st.declare(var_name, sym_info);
    visit(node->children[2].get());
    st.exit_scope();
    if (!builder->GetInsertBlock()->getTerminator()) {
      llvm::Value *cur = builder->CreateLoad(llvm::Type::getInt64Ty(*context), alloca, var_name);
      builder->CreateStore(builder->CreateAdd(cur, step_val, "nextval"), alloca);
      builder->CreateBr(cond_bb);
    }
    builder->SetInsertPoint(after_bb);
  } else {
    llvm::Value *tensor_val = visit(iter_node);
    llvm::Function *getF = module->getFunction("zorn_tensor_get");
    if (!getF) {
      llvm::FunctionType *FT = llvm::FunctionType::get(
          llvm::Type::getDoubleTy(*context),
          {llvm::PointerType::getUnqual(*context), llvm::Type::getInt64Ty(*context), llvm::PointerType::getUnqual(*context)}, false);
      getF = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "zorn_tensor_get", module.get());
    }
    llvm::Function *shapeF = module->getFunction("zorn_tensor_shape");
    if (!shapeF) {
      llvm::FunctionType *FT = llvm::FunctionType::get(
          llvm::PointerType::getUnqual(*context), {llvm::PointerType::getUnqual(*context)}, false);
      shapeF = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "zorn_tensor_shape", module.get());
    }
    llvm::Value *shape_tensor = builder->CreateCall(shapeF, {tensor_val});
    llvm::Value *one_val = llvm::ConstantInt::get(*context, llvm::APInt(64, 1, true));
    llvm::Value *zero_val = llvm::ConstantInt::get(*context, llvm::APInt(64, 0, true));
    llvm::AllocaInst *idx_arr = builder->CreateAlloca(llvm::Type::getInt64Ty(*context), one_val);
    builder->CreateStore(zero_val, idx_arr);
    llvm::Value *size_dbl = builder->CreateCall(getF, {shape_tensor, one_val, idx_arr}, "sizetmp");
    llvm::Value *size_int = builder->CreateFPToSI(size_dbl, llvm::Type::getInt64Ty(*context), "sizeint");
    llvm::AllocaInst *counter = create_entry_block_alloca(the_function, var_name + ".idx", llvm::Type::getInt64Ty(*context));
    builder->CreateStore(zero_val, counter);
    bool is_float_elem = iter_node->type.base_type == TypeNode::FLOAT;
    llvm::Type *elem_type = is_float_elem ? llvm::Type::getDoubleTy(*context) : llvm::Type::getInt64Ty(*context);
    llvm::AllocaInst *elem_alloca = create_entry_block_alloca(the_function, var_name, elem_type);
    llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(*context, "foreach_cond", the_function);
    llvm::BasicBlock *loop_bb = llvm::BasicBlock::Create(*context, "foreach_body", the_function);
    llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(*context, "foreach_after", the_function);
    builder->CreateBr(cond_bb);
    builder->SetInsertPoint(cond_bb);
    llvm::Value *cur_idx = builder->CreateLoad(llvm::Type::getInt64Ty(*context), counter, var_name + ".idx");
    builder->CreateCondBr(builder->CreateICmpSLT(cur_idx, size_int, "foreach_cond"), loop_bb, after_bb);
    builder->SetInsertPoint(loop_bb);
    builder->CreateStore(cur_idx, idx_arr);
    llvm::Value *elem_dbl = builder->CreateCall(getF, {tensor_val, one_val, idx_arr}, "elemtmp");
    llvm::Value *elem_val = is_float_elem ? elem_dbl : builder->CreateFPToSI(elem_dbl, llvm::Type::getInt64Ty(*context), "elemint");
    builder->CreateStore(elem_val, elem_alloca);
    st.enter_scope();
    Type var_type = Type::from_type_node(iter_node->type.base_type);
    SymbolInfo sym_info(var_type, false);
    sym_info.llvm_value = elem_alloca;
    st.declare(var_name, sym_info);
    visit(node->children[2].get());
    st.exit_scope();
    if (!builder->GetInsertBlock()->getTerminator()) {
      llvm::Value *next_idx = builder->CreateAdd(cur_idx, one_val, "nextidx");
      builder->CreateStore(next_idx, counter);
      builder->CreateBr(cond_bb);
    }
    builder->SetInsertPoint(after_bb);
  }
  return nullptr;
}
