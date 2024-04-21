#pragma once

typedef struct {
  char *host;
  int port;
} rshsh_client_ctx;

/// @brief Remote ShSh Client
/// @param ctx -- client context
void rshsh_client(rshsh_client_ctx ctx);
