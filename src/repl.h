#pragma once

#include <stdio.h>

typedef struct {
  FILE *in;
} shsh_repl_ctx;

/// @brief ShSh REPL
/// @param ctx -- REPL context
int shsh_repl(shsh_repl_ctx ctx);
