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

#include "codegen.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "error.h"

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModule() {
  InitializeCodeGenModule();
}

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    CodeGenerator c;
    if (auto *FnIR = FnAST->codegen(&c)) {
      std::cerr << "Read function definition:" << std::endl;
      FnIR->print(llvm::errs());
      std::cerr << "\n";
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    CodeGenerator c;
    if (auto *FnIR = ProtoAST->codegen(&c)) {
      std::cerr << "Read extern: " << std::endl;
      FnIR->print(llvm::errs());
      std::cerr << "\n";
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    CodeGenerator c;
    if (auto *FnIR = static_cast<llvm::Function*>(FnAST->codegen(&c))) {
      std::cerr << "Read top-level expression:" << std::endl;
      FnIR->print(llvm::errs());
      std::cerr << std::endl;

      // Remove the anonymous expression.
      FnIR->eraseFromParent();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
void MainLoop() {
  while (1) {
    std::cerr << "ready> ";

    if (getCurTok().value() == ';') {
            getNextToken();
            continue;
    }

    switch (getCurTok().type()) {
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

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {
  // Prime the first token.
  std::cerr <<  "ready> ";
  getNextToken();

  // Make the module, which holds all the code.
  InitializeModule();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}