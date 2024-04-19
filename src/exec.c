#include "exec.h"
#include "log.h"
#include "parser.h"
#include "types.h"
#include "utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int open_io(Command *command, int *fileIn, int *fileOut) {
  if (command->flags & CMD_FILE_IN) {
    log_debug("Opening file %s for reading\n", slice_to_str(command->in_file));
    *fileIn = open(slice_to_stack_str(command->in_file), O_RDONLY);
    if (*fileIn == -1) {
      return -1;
    }
  }

  if (command->flags & CMD_FILE_OUT) {
    log_debug("Opening file %s for writing\n", slice_to_str(command->out_file));
    *fileOut = open(slice_to_stack_str(command->out_file),
                    O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (*fileOut == -1) {
      return -1;
    }
  }

  return 0;
}

void close_io(Command *command, int fileIn, int fileOut) {
  if (command->flags & CMD_FILE_IN) {
    close(fileIn);
  }

  if (command->flags & CMD_FILE_OUT) {
    close(fileOut);
  }
}

ExecResult exec_command(Command *command) {
  log_debug("Executing command %s\n", slice_to_stack_str(command->name));
  ExecResult r = {.status = EXEC_SUCCESS, .pid = 0, .exit_code = -1};
  int fileIn = STDIN_FILENO;
  int fileOut = STDOUT_FILENO;

  if (open_io(command, &fileIn, &fileOut) == -1) {
    r.status = EXEC_ERROR_FILE_OPEN;
    return r;
  }

  pid_t main_pid = getpid();
  pid_t pid = fork();
  if (pid == -1) {
    r.status = EXEC_FORK_ERROR;
    close_io(command, fileIn, fileOut);
    return r;
  }

  if (pid == 0) {
    if (command->flags & CMD_BG) {
      log_debug("Setting process group ID to %d\n", abs(main_pid));
      setpgid(0, abs(main_pid));
    } else {
      log_debug("Setting process group ID to 0\n", NULL);
      setpgid(0, 0);
    }

    const int argc = command->args.len + /*cmd*/ 1 + /*NULL*/ 1;
    char *argv[argc];
    char *cmd = slice_to_stack_str(command->name);
    argv[0] = cmd;
    for (size_t i = 0; i < command->args.len; i++) {
      argv[i + 1] = slice_to_stack_str(command->args.data[i]);
    }
    argv[argc - 1] = NULL;

    log_debug("Child process with PID %d\n", getpid());
    if (dup2(fileIn, STDIN_FILENO) == -1) {
      panic("dup2 failed");
    }
    if (dup2(fileOut, STDOUT_FILENO) == -1) {
      panic("dup2 failed");
    }
    execvp(cmd, argv);
    log_warn("Command not found: %s\n", cmd);
    exit(1);
  }

  r.pid = pid;
  if (command->flags & CMD_BG) {
    log_debug("Background process with PID %d\n", pid);
    r.status = EXEC_IN_BACKGROUND;
    return r;
  }

  int status;
  do {
    log_debug("Waiting for child process with PID %d\n", pid);
    if (waitpid(pid, &status, 0) == -1) {
      close_io(command, fileIn, fileOut);
      panic("waitpid failed");
    }
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  r.exit_code = WEXITSTATUS(status);
  close_io(command, fileIn, fileOut);
  log_debug("Child process with PID %d exited with status %d\n", pid,
            r.exit_code);
  return r;
}
