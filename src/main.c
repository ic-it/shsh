#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  char *input = "ls -l | grep .c";

  Lexer *lexer = init_lexer(input);
  while (1) {
    Token *token = lexer_next_token(lexer);
    if (token->type == TOKEN_EOF)
      break;
    printf("Token(%d, %s)\n", token->type, token->value);
  }

  free(lexer);
  return 0;
}