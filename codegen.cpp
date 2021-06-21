/*-
 * Copyright (c) 2021 Fehmi Noyan Isi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

#include <functional>
#include <iostream>
#include <string>
#include <map>

#include "codegen.h"

static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::unique_ptr<llvm::Module> TheModule;
static std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
static std::map<std::string, llvm::Value*> NamedValues; // symbol table

void InitializeCodeGenModuleAndPassManager() {
        // Open a new context and module.
        TheContext = std::make_unique<llvm::LLVMContext>();
        TheModule = std::make_unique<llvm::Module>("my cool jit", *TheContext);

        // Create a new pass manager
        TheFPM = std::make_unique<llvm::legacy::FunctionPassManager>(TheModule.get());

        // ====== 
        // add optimisations
        // ======
        // do simple peephole opt, and bit-twiddling opt
        TheFPM->add(llvm::createInstructionCombiningPass());
        // reassociate expressions
        TheFPM->add(llvm::createReassociatePass());
        // eliminiate common subexpression
        TheFPM->add(llvm::createGVNPass());
        // simplfy the control flow graph (delete unreachable branches etc.)
        TheFPM->add(llvm::createCFGSimplificationPass());

        TheFPM->doInitialization();

        // Create a new builder for the module.
        Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}

llvm::Value *LogErrorV(std::string str) {
        return LogErrorV(str.c_str());
}

llvm::Value *LogErrorV(const char *Str) {
        LogError(Str);
        return nullptr;
}

// The code generator for numeric expression. This is very
// starightforward
llvm::Value* CodeGenerator::NumberExprCodeGen(NumberExprAST* a) {
        return llvm::ConstantFP::get(*TheContext, llvm::APFloat(a->getVal()));
}

// fetch the variable from the symbol table and return its value
llvm::Value* CodeGenerator::VariableExprCodeGen(VariableExprAST* a) {
        llvm::Value *V = NamedValues[a->getName()];
        if (!V)
                LogErrorV(std::string("Unknown variable name : " + 
                        a->getName()).c_str());
        return V;
}

// recursively calculate the value of LHS and RHS of the 
// binary experession
llvm::Value* CodeGenerator::BinaryExprCodeGen(BinaryExprAST* a) {
        llvm::Value *L = a->getLHS()->codegen(this);
        llvm::Value *R = a->getRHS()->codegen(this);

        if (!L || !R)
                return nullptr;
        
        switch(a->getOp()) {
        case '+':
                return Builder->CreateFAdd(L, R, "addtmp");
        case '-':
                return Builder->CreateFSub(L, R, "subtmp");
        case '*':
                return Builder->CreateFMul(L, R, "multmp");
        case '/':
                return Builder->CreateFDiv(L, R, "divtmp");
        case '<':
                L = Builder->CreateFCmpULT(L, R, "cmptmp");
                return Builder->CreateUIToFP(L, 
                        llvm::Type::getDoubleTy(*TheContext), "booltmp");
        default:
                return LogErrorV(std::string(": invalid binary operator.")
                        .insert(0, 1, a->getOp()));
        }

        return nullptr;
}

llvm::Value* CodeGenerator::CallsExprCodeGen(CallsExprAST* a) {
        llvm::Function *CalleeF = TheModule->getFunction(a->getCallee());
        if (!CalleeF)
                return LogErrorV(std::string(": unknown function referenced")
                        .insert(0, a->getCallee()));
        
        if (CalleeF->arg_size() != a->getArgSize())
                return LogErrorV(std::string(": incorrect # arguments passeed")
                        .insert(0, a->getCallee()));

        std::vector<llvm::Value*> ArgsV;
        for (unsigned i=0, e = a->getArgSize(); i != e; ++i) {
                ExprAST *arg = a->getArgAt(i);
                if (!arg)
                        return LogErrorV("Error while fetching function arguments");
                
                ArgsV.push_back(arg->codegen(this));
                if (!ArgsV.back())
                        return nullptr;
        }
        return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Function* CodeGenerator::PrototypeCodeGen(PrototypeAST* a) {
        std::vector<llvm::Type*> Doubles(a->getArgSize(), 
                                llvm::Type::getDoubleTy(*TheContext));

        llvm::FunctionType *FT = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*TheContext), Doubles, false);

        llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, 
                                a->getName(), *TheModule);

        unsigned idx = 0;
        for (auto &Arg: F->args())
                Arg.setName(a->getArgNameAt(idx++));
        
        return F;
}

llvm::Function* CodeGenerator::FunctionCodeGen(FunctionAST* a) {
        // First, check for an existing function from a previous 'extern' declaration.
        llvm::Function *TheFunction = TheModule->getFunction(a->getProto()->getName());

        if (!TheFunction)
                TheFunction = static_cast<llvm::Function*>(a->getProto()->codegen(this));
        
        if (!TheFunction)
                return nullptr;
        
        if (!TheFunction->empty())
                return static_cast<llvm::Function*>(LogErrorV("Function cannot be redefined."));

        // Create a new basic block to start insertion into.
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", 
                        TheFunction);
        Builder->SetInsertPoint(BB);

        // Record the function arguments in the NamedValues map.
        NamedValues.clear();
        for (auto &Arg : TheFunction->args())
                NamedValues[std::string(Arg.getName())] = &Arg;

        if (llvm::Value* RetVal = a->getBody()->codegen(this)) {
                // finish off the function
                Builder->CreateRet(RetVal);

                // validate the generated code
                llvm::verifyFunction(*TheFunction);

                // optimize the function
                TheFPM->run(*TheFunction);

                return TheFunction;
        }

        // Error reading body, remove function.
        TheFunction->eraseFromParent();
        return nullptr;
}