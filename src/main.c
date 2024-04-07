#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strnokentype[] = {
    "TOKEN_WORD",        "TOKEN_ESCAPED_WORD", "TOKEN_PIPE",
    "TOKEN_BG",          "TOKEN_STAR",         "TOKEN_REDIRECT_OUT",
    "TOKEN_REDIRECT_IN", "TOKEN_REDIRECT_AT",  "TOKEN_REDIRECT_FROM",
    "TOKEN_SEMICOLON",   "TOKEN_NEWLINE",      "TOKEN_EOF",
    "TOKEN_ERROR",
};

int main(void) {
  char *input = "ls -l | grep '*.txt' > output*.txt & ; echo hello >@"
                "192.168.1.10:8080";

  // input
  printf("Input: %s\n", input);

  Lexer lexer = {.input = input, .position = 0};
  while (1) {
    Token token = lexer_next_token(&lexer);
    if (token.type == TOKEN_EOF)
      break;
    printf("Token(%s, '%.*s')\n", strnokentype[token.type], token.length,
           input + token.position);
    if (token.type == TOKEN_ERROR)
      break;
  }

  return 0;
}
