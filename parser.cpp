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
#include "lexer.h"
#include "error.h"

static Token CurTok;

Token getCurTok() {
        return CurTok;
}

Token getNextToken() {
        return CurTok = GetTok();
}

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
        std::unique_ptr<ExprAST> LHS);

// called when the current token type is tok_number
// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
        auto Result = std::make_unique<NumberExprAST>(getNumVal());
        getNextToken();
        return std::move(Result);
}

// used to parse a parenthesised expression
// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
        getNextToken(); // consume '('
        auto V = ParseExpression();
        if (!V) 
                return nullptr;

        if (CurTok.value() != ')')
                return LogError("expected ')'");
        getNextToken(); // consume ')'
        return V;
}

// called when the current token type is tok_identifier
// constructs either a variable expression or function call expression
// identifierexpr
//   ::= identifier
//   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
        std::string IdName = getIdentifierStr();

        getNextToken(); // consume the identifier

        // Simple variable reference
        if (CurTok.value() != '(')
                return std::make_unique<VariableExprAST>(IdName);

        // Function call
        getNextToken(); // consume '('
        std::vector<std::unique_ptr<ExprAST>> Args;
        if (CurTok.value() != ')') {
                while(1) {
                        if (auto Arg = ParseExpression())
                                Args.push_back(std::move(Arg));
                        else
                                return nullptr;

                        if (CurTok.value() == ')')
                                break;
                        
                        if (CurTok.value() != ',')
                                return LogError("Expected ')' or ',' ");
                        getNextToken();
                }
        }

        getNextToken(); // consume ')'

        return std::make_unique<CallsExprAST>(IdName, std::move(Args));
}

// helper function to parse a primary expression
// primary
//   ::= identifierexpr
//   ::= numberexpr
//   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
        switch(CurTok.type()) {
                case TokenType::tok_identifier:
                        return ParseIdentifierExpr();
                case TokenType::tok_number:
                        return ParseNumberExpr();
                case TokenType::tok_lp:
                        return ParseParenExpr();
                default:
                        return LogError("unknown token when expecting an"
                                " expression");
        }
}

// parse binary expressions
static int GetTokPrecedence() {
        if (!isascii(CurTok.value()))
                return -1;
        
        switch (CurTok.value())
        {
                case '<': return 10;
                case '+': return 20;
                case '-': return 20;
                case '*': return 40;
                default: return -1;
        }
}

static std::unique_ptr<ExprAST> ParseExpression() {
        auto LHS = ParsePrimary();
        if (!LHS)
                return nullptr;
        return ParseBinOpRHS(0, std::move(LHS));
}

// binoprhs
//   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
        std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurTok.value();
    getNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    // Merge LHS/RHS.
    LHS = std::make_unique<BinaryExprAST>(BinOp, 
        std::move(LHS), std::move(RHS));
  }
}

// function prototype
// def fib(x)
//
// prototype ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
        if (CurTok.type() != TokenType::tok_identifier)
                return LogErrorP("Expected function name in prototype");
        
        std::string FnName = getIdentifierStr();
        getNextToken();

        if (CurTok.value() != '(')
                return LogErrorP("Expected '(' in prototype");
        
        // argument list
        std::vector<std::string> ArgNames;
        while (getNextToken().type() == TokenType::tok_identifier)
                ArgNames.push_back(getIdentifierStr());
        
        if (CurTok.value() != ')')
                return LogErrorP("Expected ')' in prototype");
        
        getNextToken();

        return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

// function definition
// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParseDefinition() {
        getNextToken();

        auto Proto = ParsePrototype();
        if (!Proto) return nullptr;

        if (auto E = ParseExpression())
                return std::make_unique<FunctionAST>(std::move(Proto), 
                        std::move(E));
        return nullptr;
}

// external functions
// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern() {
        getNextToken();
        return ParsePrototype();
}

// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}
