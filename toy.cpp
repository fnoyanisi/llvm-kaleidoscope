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
        TokenType type;
        int value;
        public:
                Token(TokenType t): type(t), value(-1);
                Token(TokenType t, int v): type(t), value(v);
}

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