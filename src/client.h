#pragma once

#include <stdio.h>

typedef struct {
  char *host;
  int port;
  FILE *in;
} rshsh_client_ctx;

/// @brief Remote ShSh Client
/// @param ctx -- client context
/// @return status code
int rshsh_client(rshsh_client_ctx ctx);
