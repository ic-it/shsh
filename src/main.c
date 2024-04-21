#include "exec.h"
#include "lexer.h"
#include "log.h"
#include "panic.h"
#include "parser.h"
#include "types.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

Jobs *jobs;

static void handle_sigchld(int sig __attribute__((unused))) {
  int status;
  pid_t pid;

  while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
    if (pid == -1) {
      log_error("Error: waitpid failed\n", NULL);
      return;
    }
    if (!WIFEXITED(status) && !WIFSIGNALED(status)) {
      log_info("[H] Process with PID %d exited abnormally\n", pid);
      continue;
    }
    log_info("\n[H] Process with PID %d exited with status %d\n", pid,
             WEXITSTATUS(status));
    remove_pid(jobs, pid);
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
  jobs = jobs_new();

  Lexer lexer;
  Parser parser;
  Executor executor = executor_new(NULL, jobs);

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
    executor.parser = &parser;

    while (1) {
      ExecResult er = exec_next(&executor, stdin, stdout);
      if (er.status == EXEC_PARSE_EOF) {
        break;
      }

      switch (er.status) {
      case EXEC_ERROR_FILE_OPEN:
        log_error("Unable to open file\n", NULL);
        break;
      case EXEC_PARSE_ERROR:
        log_error("Invalid Syntax\n", NULL);
        break;
      case EXEC_SEMANTIC_ERROR:
        log_error("Semantic Error: %s\n",
                  get_semantic_reason(er.semantic_reason));
        break;
      case EXEC_PARSE_EOF:
        break;
      case EXEC_SUCCESS:
        break;
      case EXEC_IN_BACKGROUND:
        break;
      case EXEC_PIPELINE:
        break;
      }
    }
  }

  jobs_free(jobs);
  return 0;
}
