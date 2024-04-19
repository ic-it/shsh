#include "exec.h"
#include "lexer.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "types.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

void handle_sigchld(int sig) {
  printf("Handling SIGCHLD %d\n", sig);
  int status;
  pid_t pid;
  while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
    if (!WIFEXITED(status) && !WIFSIGNALED(status)) {
      continue;
    }
    printf("\nPID[%d] exited with status %d\n", pid, WEXITSTATUS(status));
  }
}

// Simple Console
int main(void) {
  if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
    printf("Error: Unable to catch SIGINT\n");
    return -1;
  }
  if (signal(SIGCHLD, handle_sigchld) == SIG_ERR) {
    printf("Error: Unable to catch SIGCHLD\n");
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
        printf("\nExiting... (Ctrl + D)\n");
        return 0;
      }
      if (c == 12) { // Ctrl + L
        system("clear");
        continue;
      }
      if (c == EOF && (c = getchar()) == EOF) { // Ctrl + D (EOF)
        printf("\nExiting... (Ctrl + D)\n");
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

      ExecResult er = exec_command(&pr.command);
      switch (er.status) {
      case EXEC_ERROR_FILE_OPEN:
        printf("Error: Unable to open file\n");
        break;
      case EXEC_FORK_ERROR:
        printf("Error: Unable to fork\n");
        break;
      case EXEC_WAIT_ERROR:
        printf("Error: Unable to execute command\n");
        break;
      case EXEC_SUCCESS:
        break;
      }
      if (pr.command.flags & CMD_BG) {
        printf("Started process with PID %d\n", er.pid);
      } else {
      }
      clear_command(pr.command);
    }
  }
  return 0;
}
