#include "lexer.h"
#include "parser.h"
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
  char input[] = "ls -l | grep '*.txt' > output*.txt & ; echo hello >@"
                 "192.168.1.10:8080";
  // char input[] = "l\\s '-l \\'a\\sd' < input.txt > output.txt &";

  // input
  printf("Input: %s\n", input);

  Lexer lexer = {.input = input, .position = 0};

  while (1) {
    Token token = lex_next(&lexer);
    if (token.type == TOKEN_EOF)
      break;
    printf("Token(%s, '%.*s')\n", strnokentype[token.type], token.length,
           input + token.position);
    if (token.type == TOKEN_ERROR)
      break;
  }
  return 0;

  Parser parser = {
      .input = input,
      .lexer = &lexer,
      .current_token =
          (Token){
              .length = -1,
              .position = -1,
              .type = TOKEN_EOF,
          },
  };
  Command command = parse_next(&parser);

  if (command.result == PARSE_ERROR) {
    printf("Error: parsing failed\n");
    return 1;
  }

  printf("name: %s\n", command.name);
  for (int i = 0; i < command.argc; i++) {
    printf("argv[%d]: %s\n", i, command.argv[i]);
  }
  if (command.type & CMD_FILE_IN) {
    printf("CMD_FILE_IN: %s\n", command.in_file);
  }
  if (command.type & CMD_FILE_OUT) {
    printf("CMD_FILE_OUT: %s\n", command.out_file);
  }
  if (command.type & CMD_TCP_IN) {
    printf("CMD_TCP_IN: %s\n", command.in_tcp);
  }
  if (command.type & CMD_TCP_OUT) {
    printf("CMD_TCP_OUT: %s\n", command.out_tcp);
  }
  if (command.type & CMD_PIPE) {
    printf("CMD_PIPE\n");
  }
  if (command.type & CMD_BG) {
    printf("CMD_BG\n");
  }

  return 0;
}
