#pragma once

typedef struct {
  char *host;
  int port;
} rshsh_server_ctx;

/// @brief Remote ShSh Server
/// @param ctx -- server context
/// @return int -- status code
int rshsh_server(rshsh_server_ctx ctx);