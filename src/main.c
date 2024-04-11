#include "exec.h"
#include "lexer.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "types.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void handle_sigint(int sig __attribute__((unused))) { printf("\n"); }

int setup_shell(void) {
  if (signal(SIGINT, handle_sigint) == SIG_ERR) {
    printf("Error: Unable to catch SIGINT\n");
    return -1;
  }
  return 0;
}

// Simple Console
int main(void) {
  if (setup_shell() == -1) {
    return -1;
  }

  char input[1024];
  Lexer lexer;
  Parser parser;
  ParseResult pr;
  SemanticResult sr;

  while (1) {
    printf(">> ");
    int i = 0;
    char c;
    while (1) {
      c = getchar();
      if (c == '\n') { // Enter
        input[i] = '\0';
        break;
      }
      if (c == 4) { // Ctrl + D
        printf("\n");
        printf("Exiting...\n");
        return 0;
      }
      if (c == 12) { // Ctrl + L
        system("clear");
        continue;
      }
      if (c == EOF) {
        printf("\n");
        printf("Exiting...\n");
        return 0;
      }
      input[i++] = c;
    }
    input[i] = '\0';

    lexer = lex_new(input);
    parser = parse_new(&lexer);

    while (1) {
      pr = parse_next(&parser);

      if (pr.result == PARSE_EOF) {
        break;
      }

      if (pr.result == PARSE_ERROR) {
        printf("Invalid Syntax\n");
        clear_command(pr.command);
        continue;
      }

      sr = semantic_analyze(&pr.command);
      if (sr.result == SEMANTIC_ERROR) {
        printf("Semantic Error: %s\n", error_reasons[sr.reason]);
        clear_command(pr.command);
        continue;
      }

      exec_command(&pr.command);

      clear_command(pr.command);
    }
  }
  return 0;
}
