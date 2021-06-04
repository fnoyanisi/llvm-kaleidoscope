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

enum class TokenType {
        tok_eof,

        // commands
        tok_def,
        tok_extern,

        // primary
        tok_identifier,
        tok_number,

        // operator
        tok_op,

        // undefinied
        tok_undef,

        // left paranthesis
        tok_lp,

        // semi-colon
        tok_sc,
};

class Token {
        TokenType _type;
        int _value;
public:
        Token(TokenType t = TokenType::tok_undef, int v = -1): 
                _type{t}, _value{v} {};
        const TokenType type() const { return _type; }
        void type(TokenType t) { _type = t; }
        const int value() const { return _value; }
        void value(int v) { _value = v; }
};

static std::string IdentifierStr; // used when the token is an identifier
static double NumVal;             // used when the token is a number 

// returns the next token from standard input
static Token GetTok() {
        static int LastChar = ' ';

        // skip whitespaces
        while (isspace(LastChar))
                LastChar = getchar();
        
        // identifiers and keywords
        if (isalpha(LastChar)) {
                IdentifierStr = LastChar;
                while (isalnum((LastChar = getchar())))
                        IdentifierStr += LastChar;
                
                if (IdentifierStr == "def")
                        return Token(TokenType::tok_def);
                else if (IdentifierStr == "extern")
                        return Token(TokenType::tok_extern);
                else if (IdentifierStr == "(")
                        return Token(TokenType::tok_lp, '(');
                else if (IdentifierStr == ";")
                        return Token(TokenType::tok_lp, ';');
                else
                        return Token(TokenType::tok_identifier);
        }

        // numerical values
        if (isdigit(LastChar) || LastChar == '.') {
                std::string NumStr;
        
                do {
                        NumStr += LastChar;
                        LastChar = getchar();
                } while (isdigit(LastChar) || LastChar == '.');

                try {
                        NumVal = std::stod(NumStr);
                } catch (const std::invalid_argument&) {
                        std::cerr << "Invalid numeric value: " 
                                << NumVal << std::endl;
                        throw;
                } catch (const std::out_of_range&) {
                        std::cerr << "Numeric value is out of range: " 
                                << NumVal << std::endl;
                        throw;
                }
                return Token(TokenType::tok_number, NumVal);
        }

        // comments
        if (LastChar == '#') {
                do {
                        LastChar = getchar();
                } while (LastChar != EOF && LastChar != '\n' && 
                                LastChar != '\r');

                if (LastChar != EOF)
                        return GetTok();
        }

        // end of the file
        if (LastChar == EOF)
                return TokenType::tok_eof;

        // return the ASCII value of the character
        int ThisChar = LastChar;
        LastChar = getchar();
        return Token(TokenType::tok_op, ThisChar);
}

static Token CurTok;
static Token getNextToken() {
        return CurTok = GetTok();
}

std::unique_ptr<ExprAST> LogError(const char *Str) {
        std::cerr << "LogError : " << Str << std::endl;
        return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
        LogError(Str);
        return nullptr;
}

// Parser code

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
        std::unique_ptr<ExprAST> LHS);

// called when the current token type is tok_number
// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
        auto Result = std::make_unique<NumberExprAST>(NumVal);
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
        std::string IdName = IdentifierStr;

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
        
        std::string FnName = IdentifierStr;
        getNextToken();

        if (CurTok.value() != '(')
                return LogErrorP("Expected '(' in prototype");
        
        // argument list
        std::vector<std::string> ArgNames;
        while (getNextToken().type() == TokenType::tok_identifier)
                ArgNames.push_back(IdentifierStr);
        
        if (CurTok.value() != ')')
                return LogErrorP("Expected ')' in prototype");
        
        getNextToken();

        return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

// function definition
// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
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
static std::unique_ptr<PrototypeAST> ParseExtern() {
        getNextToken();
        return ParsePrototype();
}

// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

static void HandleDefinition() {
  if (ParseDefinition()) {
    std::cerr << "Parsed a function definition.\n";
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (ParseExtern()) {
    std::cerr <<  "Parsed an extern\n";
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (ParseTopLevelExpr()) {
    std::cerr << "Parsed a top-level expr\n";
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (1) {
    std::cerr << "ready> ";

    if (CurTok.value() == ';') {
            getNextToken();
            continue;
    }

    switch (CurTok.type()) {
    case TokenType::tok_eof:
      return;
    case TokenType::tok_def:
      HandleDefinition();
      break;
    case TokenType::tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}

int main() {
  // Prime the first token.
  std::cerr <<  "ready> ";
  getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}