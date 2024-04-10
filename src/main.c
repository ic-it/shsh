#include "lexer.h"
#include "parser.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strtokentype[] = {
    "TOKEN_WORD",      "TOKEN_ESCAPED_WORD", "TOKEN_PIPE",    "TOKEN_BG",
    "TOKEN_FILE_OUT",  "TOKEN_FILE_IN",      "TOKEN_TCP_OUT", "TOKEN_TCP_IN",
    "TOKEN_SEMICOLON", "TOKEN_NEWLINE",      "TOKEN_EOF",     "TOKEN_ERROR",
};

int main(void) {
  char input[] =
      "ls -l | grep 'hello *.txt' > output\\ *.txt & \n echo hello >@"
      "192.168.1.10:8080 < input.txt &";
  // char input[] = "l\\s '-l \\'a\\sd' < input.txt > output.txt &";

  // input
  printf("Input: %s\n", input);

  Lexer lexer = {.input = input, .position = 0};

  Parser parser = {
      .input = input,
      .lexer = &lexer,
      .current_token =
          (Token){
              .type = TOKEN_ERROR,
              .value = (Slice){.data = NULL, .pos = 0, .len = 0},
          },
  };

  while (1) {
    printf("\t\t***\n");
    ParseResult pr = parse_next(&parser);

    if (pr.result == PARSE_EOF) {
      printf("EOF\n");
      break;
    }

    if (pr.result == PARSE_ERROR) {
      m_print_slice("Invalid Syntax", parser.current_token.value);
      clear_command(pr.command);
      return 1;
    }

    m_print_slice("name", pr.command.name);
    m_print_slicev("args", pr.command.args);
    if (pr.command.type & CMD_FILE_IN) {
      m_print_slice("in_file", pr.command.in_file);
    }
    if (pr.command.type & CMD_FILE_OUT) {
      m_print_slice("out_file", pr.command.out_file);
    }
    if (pr.command.type & CMD_TCP_IN) {
      m_print_slice("in_tcp", pr.command.in_tcp);
    }
    if (pr.command.type & CMD_TCP_OUT) {
      m_print_slice("out_tcp", pr.command.out_tcp);
    }
    if (pr.command.type & CMD_PIPE) {
      printf("CMD_PIPE\n");
    }
    if (pr.command.type & CMD_BG) {
      printf("CMD_BG\n");
    }

    clear_command(pr.command);
  }
  return 0;
}
