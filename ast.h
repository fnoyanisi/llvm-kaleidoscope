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

#ifndef _AST_H_
#define _AST_H_

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>

// Base class for all expression nodes
class ExprAST {
public:
        virtual ~ExprAST() {}
        virtual void print() {}
};

// Expression class for numeric literals such as "2.1"
class NumberExprAST : public ExprAST {
        double Val;
public:
        NumberExprAST(double v): Val{v} {}
        void print();
};

// Expression class for referencing a variable like "a"
class VariableExprAST : public ExprAST {
        std::string Name;
public:
        VariableExprAST(const std::string &n): Name{n} {}
        void print();
};

// Expression class for binary operators
class BinaryExprAST : public ExprAST {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;
public:
        BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs):
                Op{op}, LHS{std::move(lhs)}, RHS{std::move(rhs)} {}
        void print();
};

// Expression class for function calls
class CallsExprAST : public ExprAST {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;
public:
        CallsExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args): 
                Callee{callee}, Args{std::move(args)} {}
        void print();
};

// This class represents the "prototype" for a function,
// which captures its name, and its argument names (thus implicitly the number
// of arguments the function takes).
class PrototypeAST {
        std::string Name;
        std::vector<std::string> Args;
public:
        PrototypeAST(const std::string &name, std::vector<std::string> args):
                Args{std::move(args)}, Name{name} {}
        const std::string &getName() const { return Name; }
};

// Function definition itself
class FunctionAST {
        std::unique_ptr<PrototypeAST> Proto;
        std::unique_ptr<ExprAST> Body;
public:
        FunctionAST(std::unique_ptr<PrototypeAST> p,
                std::unique_ptr<ExprAST> b): 
                Proto{std::move(p)}, Body{std::move(b)} {}
};

#endif