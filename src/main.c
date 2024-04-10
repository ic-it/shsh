#include "lexer.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  // char input[] =
  //     "ls -l | grep -i 'hello *.txt' > output\\ *.txt &; echo hello <@"
  //     "192.168.1.10:8080 > input.txt &";
  char input[] = "#!/bin/shsh\n"
                 "rm -rf /usr/local/go \n"
                 "# Download and extract go 1.22.2\n"
                 "tar -C /usr/local -xzf go1.22.2.linux-amd64.tar.gz";

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

    SemanticResult sr = semantic_analyze(&pr.command);
    if (sr.result == SEMANTIC_ERROR) {
      printf("Semantic Error: %s\n", error_reasons[sr.reason]);
      clear_command(pr.command);
      return 1;
    }

    printf("Command:\n");
    m_print_slice("name", pr.command.name);
    m_print_slicev("args", pr.command.args);
    if (pr.command.flags & CMD_FILE_IN) {
      m_print_slice("in_file", pr.command.in_file);
    }
    if (pr.command.flags & CMD_FILE_OUT) {
      m_print_slice("out_file", pr.command.out_file);
    }
    if (pr.command.flags & CMD_TCP_IN) {
      m_print_slice("in_tcp", pr.command.in_tcp);
    }
    if (pr.command.flags & CMD_TCP_OUT) {
      m_print_slice("out_tcp", pr.command.out_tcp);
    }
    if (pr.command.flags & CMD_PIPE) {
      printf("CMD_PIPE\n");
    }
    if (pr.command.flags & CMD_BG) {
      printf("CMD_BG\n");
    }

    clear_command(pr.command);
  }
  return 0;
}
