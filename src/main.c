#include "exec.h"
#include "lexer.h"
#include "log.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "types.h"
#include "utils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static void handle_sigchld(int sig __attribute__((unused))) {
  log_debug("Caught SIGCHLD\n", NULL);
  int status;
  pid_t pid;

  while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
    if (!WIFEXITED(status) && !WIFSIGNALED(status)) {
      continue;
    }
    log_info("Process with PID %d exited with status %d\n", pid,
             WEXITSTATUS(status));
  }
  signal(SIGCHLD, handle_sigchld); // What the fuck is this?
}

// Simple Console
int main(void) {
  if (signal(SIGCHLD, handle_sigchld) == SIG_ERR) {
    panic("Error: Unable to catch SIGCHLD\n");
  }
  if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
    panic("Error: Unable to catch SIGINT\n");
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
      if (c == EOF && feof(stdin)) { // Ctrl + D (EOF)
        printf("\nExiting... (Ctrl + D)\n");
        return 0;
      } else if (c == EOF) {
        continue;
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
        log_error("Invalid Syntax\n", NULL);
        clear_command(pr.command);
        continue;
      }

      if (pr.command.name.len == 0) {
        clear_command(pr.command);
        continue;
      }

      sr = semantic_analyze(&pr.command);
      if (sr.result == SEMANTIC_ERROR) {
        log_error("Semantic Error: %s\n", error_reasons[sr.reason]);
        clear_command(pr.command);
        continue;
      }

      ExecResult er = exec_command(&pr.command);
      switch (er.status) {
      case EXEC_ERROR_FILE_OPEN:
        log_error("Unable to open file\n", NULL);
        break;
      case EXEC_FORK_ERROR:
        log_error("Unable to fork\n", NULL);
        break;
      case EXEC_WAIT_ERROR:
        log_error("Unable to wait for child process\n", NULL);
        break;
      case EXEC_SUCCESS:
        break;
      case EXEC_IN_BACKGROUND:
        break;
      }
      if (pr.command.flags & CMD_BG) {
        log_info("Started process with PID %d\n", er.pid);
      }
      clear_command(pr.command);
    }
  }
  return 0;
}
