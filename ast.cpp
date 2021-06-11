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

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>

#include "ast.h"
#include "codegen.h"

void NumberExprAST::print() const { 
        std::cout << "Numeric Expression with value " << Val << std::endl;
}

llvm::Value* NumberExprAST::codegen(CodeGenerator *c) {
        return c->NumberExprCodeGen(this);
}

void VariableExprAST::print() const { 
        std::cout << "Variable Expression with name " << Name << std::endl; 
}

llvm::Value* VariableExprAST::codegen(CodeGenerator *c) {
        return c->VariableExprCodeGen(this);
}

void BinaryExprAST::print() const { 
        std::cout << "Binary Expression '" << Op << "' with [";
        LHS->print();
        std::cout << "] on the left-hand side and [";
        RHS->print();
        std::cout << "] on the right-hand side.";
}

llvm::Value* BinaryExprAST::codegen(CodeGenerator *c) {
        return c->BinaryExprCodeGen(this);
}

ExprAST* CallsExprAST::getArgAt(unsigned i) {
        try {
                return Args.at(i).get();
        } catch (std::out_of_range const& e) {
                return nullptr;
        }
}

// Expression class for function calls
void CallsExprAST::print() const {
        std::cout << "Calling function '" << Callee << "' with " <<
        "arguments ";
        for (const std::unique_ptr<ExprAST>& e: Args) {
                std::cout << "\t";
                e->print();
                std::cout << std::endl;
        }
}

llvm::Value* CallsExprAST::codegen(CodeGenerator *c) {
        return c->CallsExprCodeGen(this);
}

llvm::Value* PrototypeAST::codegen(CodeGenerator *c) {
        return c->PrototypeCodeGen(this);
}

void PrototypeAST::print() const {
        std::cout << "Function prototype for \"" << Name << "\"" << std::endl;
}

llvm::Value* FunctionAST::codegen(CodeGenerator *c) {
        return c->FunctionCodeGen(this);
}

void FunctionAST::print() const {
        std::cout << "Function definition for\"" << Proto->getName() 
                << "\"" << std::endl;
}

std::unique_ptr<PrototypeAST> FunctionAST::getProto() {
        return std::move(Proto);
}