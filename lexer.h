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

#ifndef _TOY_LEXER_H_
#define _TOY_LEXER_H_

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>

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

Token GetTok();
std::string getIdentifierStr();
double getNumVal();

#endif