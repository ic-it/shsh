#pragma once

typedef struct {
  char *host;
  int port;
} rshsh_server_ctx;

/// @brief Remote ShSh Server
/// @param ctx -- server context
void rshsh_server(rshsh_server_ctx ctx);