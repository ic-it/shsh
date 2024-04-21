#include "repl.h"
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

Jobs *repl_jobs;

static void repl_handle_sigchld(int sig __attribute__((unused))) {
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
    log_info("[H] Process with PID %d exited with status %d\n", pid,
             WEXITSTATUS(status));
    remove_pid(repl_jobs, pid);
  }
  signal(SIGCHLD, repl_handle_sigchld); // WTF? Why do we need this?
}

int shsh_repl(shsh_repl_ctx ctx) {
  log_debug("Running REPL\n", NULL);
  FILE *in = stdin;
  if (ctx.in != NULL) {
    in = ctx.in;
  }

  if (signal(SIGCHLD, repl_handle_sigchld) == SIG_ERR) {
    panic("Error: Unable to catch SIGCHLD\n");
  }
  if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
    panic("Error: Unable to catch SIGINT\n");
  }

  char input[1024];
  repl_jobs = jobs_new();

  Lexer lexer;
  Parser parser;
  Executor executor = executor_new(NULL, repl_jobs);

  while (1) {
    printf(">> ");
    int i = 0;
    char c;
    while (1) {
      // read from in
      c = fgetc(in);
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
      if (c == EOF && feof(in)) { // Ctrl + D (EOF)
        if (i != 0) {
          input[i] = '\0';
          break;
        }
        printf("\nExiting... (Ctrl + D)\n");
        return 0;
      } else if (c == EOF) {
        continue;
      }
      input[i++] = c;
      if (ctx.in != NULL) {
        putchar(c);
      }
    }
    if (ctx.in != NULL) {
      putchar('\n');
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

  jobs_free(repl_jobs);
  return 0;
}
