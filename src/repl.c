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

typedef enum {
  REPL_PHR_EXIT = 1,
} REPLPrehookResult;

int repl_prehook(Command cmd) {
  char *cmd_name = slice_to_stack_str(cmd.name);
  if (strcmp(cmd_name, "exit") == 0) {
    log_debug("Exiting REPL\n", NULL);
    return REPL_PHR_EXIT;
  }
  return 0;
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

  char input[1024 * 3]; // 3KB
  repl_jobs = jobs_new();

  Lexer lexer;
  Parser parser;
  Executor executor = executor_new(NULL, repl_jobs);

  bool is_eof = false;
  while (!is_eof) {
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
        is_eof = true;
        break;
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
      ExecResult er =
          exec_next(&executor, STDIN_FILENO, STDOUT_FILENO, repl_prehook);
      if (er.status == EXEC_PARSE_EOF) {
        break;
      }

      if (er.status == EXEC_PREHOOK_BREAK) {
        if (er.prehook_result == REPL_PHR_EXIT) {
          is_eof = true;
          break;
        }
        panic("Unknown prehook result\n");
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
      case EXEC_PREHOOK_BREAK:
        break;
      }
    }
  }
  for (int i = 0; i < repl_jobs->pids_size; i++) {
    if (repl_jobs->pids[i] != -1) {
      log_info("Waiting for PID %d\n", repl_jobs->pids[i]);
      waitpid(repl_jobs->pids[i], NULL, 0);
    }
  }
  jobs_free(repl_jobs);

  return 0;
}
