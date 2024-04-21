#include "exec.h"
#include "log.h"
#include "panic.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "types.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

Executor executor_new(Parser *parser, Jobs *jobs) {
  return (Executor){.parser = parser, .jobs = jobs};
}

Jobs *jobs_new(void) {
  Jobs *j = malloc(sizeof(Jobs));
  j->pids = NULL;
  j->pids_size = 0;
  j->pids_capacity = 0;
  assertf(pthread_mutex_init(&j->mutex, NULL) == 0, "mutex init failed", NULL);
  return j;
}

void jobs_free(Jobs *jobs) {
  free(jobs->pids);
  free(jobs);
}

size_t push_pid(Jobs *jobs, pid_t pid, bool next) {
  assertf(pthread_mutex_lock(&jobs->mutex) == 0, "mutex lock failed", NULL);
  if (!next) {
    jobs->pids_size = 0;
    for (size_t i = 0; (i < jobs->pids_size); i++) {
      if (jobs->pids[i] == -1) {
        jobs->pids[i] = pid;
        assertf(pthread_mutex_unlock(&jobs->mutex) == 0, "mutex unlock failed",
                NULL);
        return i;
      }
    }
  }
  int i = jobs->pids_size;

  if (jobs->pids_size == jobs->pids_capacity) {
    jobs->pids_capacity =
        jobs->pids_capacity == 0 ? 1 : jobs->pids_capacity * 2;
    jobs->pids = realloc(jobs->pids, jobs->pids_capacity * sizeof(pid_t));
  }
  jobs->pids[jobs->pids_size++] = pid;
  assertf(pthread_mutex_unlock(&jobs->mutex) == 0, "mutex unlock failed", NULL);
  return i;
}

void remove_pid(Jobs *jobs, pid_t pid) {
  assertf(pthread_mutex_lock(&jobs->mutex) == 0, "mutex lock failed", NULL);
  for (size_t i = 0; i < jobs->pids_size; i++) {
    if (jobs->pids[i] == pid) {
      jobs->pids[i] = -1;
      break;
    }
  }
  assertf(pthread_mutex_unlock(&jobs->mutex) == 0, "mutex unlock failed", NULL);
}

ExecResult exec_next(Executor *executor) {
  ExecResult r = {
      .status = EXEC_SUCCESS,
      .exit_code = -1,
      .is_background = false,
      .is_pipeline = false,
  };

  int jobs_range[2] = {-1, -1}; // Start and end of jobs in current pipeline
  int pipe_in = -1;             // Pipe input

  while (true) { // Loop Until Command or Pipeline
    ParseResult pr = parse_next(executor->parser);

    if (strcmp(slice_to_stack_str(pr.command.name), "jobs") == 0) {
      for (size_t i = 0; i < executor->jobs->pids_size; i++) {
        if (executor->jobs->pids[i] != -1) {
          log_info("PID: %d\n", executor->jobs->pids[i]);
        }
      }
      continue;
    }

    switch (pr.result) {
    case PARSE_OK:
      break;
    case PARSE_ERROR:
      r.status = EXEC_PARSE_ERROR;
      clear_command_args(pr.command);
      // FIXME: Clear pipeline and next commands
      return r;
    case PARSE_EOF:
      assertf(pipe_in == -1, "Unexpected EOF while parsing pipeline", NULL);
      r.status = EXEC_PARSE_EOF;
      clear_command_args(pr.command);
      return r;
    default:
      panicf("Unexpected parse result: %d\n", pr.result);
    }

    if (pr.command.name.len == 0) {
      continue;
    }

    SemanticResult sr = semantic_analyze(&pr.command);
    if (sr.result != SEMANTIC_OK) {
      r.status = EXEC_SEMANTIC_ERROR;
      r.semantic_reason = sr.reason;
      return r;
    }

    r.is_background = r.is_background || CMDISBG(pr.command);
    r.is_pipeline = r.is_pipeline || CMDISPIPE(pr.command);

    // Move Arguments to Stack
    const int argc = pr.command.args.len + /*cmd*/ 1 + /*NULL*/ 1;
    char *argv[argc];
    char *cmd = slice_to_stack_str(pr.command.name);
    argv[0] = cmd;
    for (size_t i = 0; i < pr.command.args.len; i++) {
      argv[i + 1] = slice_to_stack_str(pr.command.args.data[i]);
    }
    argv[argc - 1] = NULL;

    clear_command_args(pr.command);

    int pipefd[2];
    if (CMDISPIPE(pr.command)) {
      assertf(pipe(pipefd) != -1, "pipe failed", NULL);
    }

    pid_t main_pid = getpid();
    pid_t pid = fork();
    assertf(pid != -1, "fork failed", NULL);
    if (pid == 0) {
      if (r.is_background) {
        setpgid(0, abs(main_pid));
      } else {
        setpgid(0, 0);
      }

      if (CMDISTIN(pr.command) || CMDISTOUT(pr.command)) {
        not_implemented(); // TCP IN/OUT not implemented
      }

      if (pipe_in != -1) {
        assertf(dup2(pipe_in, STDIN_FILENO) != -1, "dup2 failed", NULL);
        close(pipe_in);
      } else if (CMDISFIN(pr.command)) {
        int filein = open(slice_to_stack_str(pr.command.in_file), O_RDONLY);
        assertf(dup2(filein, STDIN_FILENO) != -1, "dup2 failed", NULL);
        close(filein);
      }

      if (CMDISPIPE(pr.command)) {
        assertf(dup2(pipefd[1], STDOUT_FILENO) != -1, "dup2 failed", NULL);
        close(pipefd[1]);
      } else if (CMDISFOUT(pr.command)) {
        int fileout = open(slice_to_stack_str(pr.command.out_file),
                           O_WRONLY | O_CREAT | O_TRUNC, 0644);
        assertf(dup2(fileout, STDOUT_FILENO) != -1, "dup2 failed", NULL);
        close(fileout);
      }

      execvp(cmd, argv);
      log_warn("Command not found: %s\n", cmd);
      _exit(1);
    }

    if (CMDISPIPE(pr.command)) {
      close(pipefd[1]);
      pipe_in = pipefd[0];
    }

    size_t jobid = push_pid(executor->jobs, pid, r.is_pipeline);
    if (jobs_range[0] == -1) {
      jobs_range[0] = jobid;
    }
    jobs_range[1] = jobid + 1;

    if (!CMDISPIPE(pr.command)) {
      break;
    }
  }

  if (r.is_background) {
    log_debug("Running in background\n", NULL);
    pid_t main_pid = getpid();
    for (int i = jobs_range[0]; i < jobs_range[1]; i++) {
      pid_t pid = executor->jobs->pids[i];
      if (pid == -1) {
        log_debug("Skipping PID %d\n", pid);
        continue;
      }

      int status;
      bool is_process_running;
      if (waitpid(pid, &status, WNOHANG) == 0) {
        is_process_running = true;
      } else {
        is_process_running = false;
      }

      if (!is_process_running) {
        log_debug("Removing PID %d\n", pid);
        remove_pid(executor->jobs, pid);
        continue;
      } else {
        log_debug("Setting PGID for PID %d\n", pid);
        setpgid(pid, abs(main_pid));
      }
    }
    return r;
  }

  log_debug("Waiting for jobs from %d to %d\n", jobs_range[0], jobs_range[1]);
  for (int i = jobs_range[0]; i < jobs_range[1]; i++) {
    int status;
    pid_t pid = executor->jobs->pids[i];
    if (pid == -1) {
      log_debug("Skipping PID %d\n", pid);
      continue;
    }
    do {
      log_debug("Waiting for PID %d\n", pid);
      pid_t wpid = waitpid(pid, &status, WUNTRACED);
      if (WIFSTOPPED(status)) {
        log_debug("PID %d stopped\n", pid);
        break;
      }
      if (WIFSIGNALED(status)) {
        log_debug("PID %d signaled\n", pid);
        break;
      }
      if (WIFEXITED(status)) {
        log_debug("PID %d exited\n", pid);
        break;
      }
      if (wpid == -1) {
        panic("waitpid failed");
      }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    if (!r.is_pipeline) {
      r.exit_code = WEXITSTATUS(status);
    }
    remove_pid(executor->jobs, pid);
  }
  return r;
}
