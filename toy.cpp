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
};

class Token {
        TokenType _type;
        int _value;
public:
        Token(TokenType t): _type(t), _value(-1);
        Token(TokenType t, int v): _type(t), _value(v);
        const TokenType type() const { return _type; }
        void type(TokenType t) { _type = t; }
        const int value() const { return _value; }
        void value(int v) { _value = v; }
};

static std::string IdentifierStr; // used when the token is an identifier
static double NumVal;             // used when the token is a number 

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
        NumberExprAST(double v): Val(v) {}
        void print() { std::cout << "Numeric Expression with value " <<
                Val << std::endl; }
};

// Expression class for referencing a variable like "a"
class VariableExprAST : public ExprAST {
        std::string Name;
public:
        VariableExprAST(const std::string &n): Name(n) {}
        void print() { std::cout << "Variable Expression with name " <<
                Name << std::endl; }
};

// Expression class for binary operators
class BinaryExprAST : public ExprAST {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;
public:
        BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs):
                Op(op), LHS(std::move(lhs)), RHS(std::move(rhs)) {}
        void print() { 
                std::cout << "Binary Expression '" << Op << "' with [";
                LHS->print();
                std::cout << "] on the left-hand side and [";
                RHS->print();
                std::cout << "] on the right-hand side.";
        }
};

// Expression class for function calls
class CallsExprAST : public ExprAST {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;
public:
        CallsExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args): 
                Callee(callee), Args(std::move(args)) {}
        void print() {
                std::cout << "Calling function '" << Callee << "' with " <<
                "arguments ";
                for (std::unique_ptr<ExprAST>& e: Args) {
                        std::cout << "\t";
                        e->print();
                        std::cout << std::endl;
                }
        }
};

// This class represents the "prototype" for a function,
// which captures its name, and its argument names (thus implicitly the number
// of arguments the function takes).
class PrototypeAST {
        std::string Name;
        std::vector<std::string> Args;
public:
        PrototypeAST(const std::string &name, std::vector<std::string> args):
                Args(std::move(args)), Name(name) {}
        const std::string &getName() const { return Name; }
};

// Function definition itself
class FunctionAST {
        std::unique_ptr<PrototypeAST> Proto;
        std::unique_ptr<ExprAST> Body;
public:
        FunctionAST(std::unique_ptr<PrototypeAST> p,
                std::unique_ptr<ExprAST> b): 
                Proto(std::move(p)), Body(std::move(b)) {}
};

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
                return Token::tok_number;
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
                return Token::tok_eof;

        // return the ASCII value of the character
        int ThisChar = LastChar;
        LastChar = getchar();
        return Token(TokenType::tok_op, ThisChar);
}