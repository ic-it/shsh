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
    log_debug("Opening file %s for reading\n",
              slice_to_stack_str(command->in_file));
    *fileIn = open(slice_to_stack_str(command->in_file), O_RDONLY);
    if (*fileIn == -1) {
      return -1;
    }
  }

  if (command->flags & CMD_FILE_OUT) {
    log_debug("Opening file %s for writing\n",
              slice_to_stack_str(command->out_file));
    *fileOut = open(slice_to_stack_str(command->out_file),
                    O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (*fileOut == -1) {
      return -1;
    }
  }

  return 0;
}

void close_io(int fileIn, int fileOut) {
  if (fileIn != STDIN_FILENO) {
    close(fileIn);
  }

  if (fileOut != STDOUT_FILENO) {
    close(fileOut);
  }
}

ExecResult exec_command(Command *command, const int pipe_in) {
  log_debug("Executing command %s\n", slice_to_stack_str(command->name));
  ExecResult r = {
      .status = EXEC_SUCCESS,
      .pid = 0,
      .exit_code = -1,
      .pipe = -1,
  };
  int fileIn = STDIN_FILENO;
  int fileOut = STDOUT_FILENO;

  if (pipe_in != -1) {
    log_debug("Setting pipe input to %d\n", pipe_in);
    fileIn = pipe_in;
  }

  int pipefd[2];
  if (command->flags & CMD_PIPE) {
    log_debug("Creating pipe\n", NULL);
    if (pipe(pipefd) == -1) {
      r.status = EXEC_PIPE_ERROR;
      return r;
    }
    log_debug("pipefd[0] = %d, pipefd[1] = %d\n", pipefd[0], pipefd[1]);
    fileOut = pipefd[1];
    r.pipe = pipefd[0];
  }

  if (open_io(command, &fileIn, &fileOut) == -1) {
    r.status = EXEC_ERROR_FILE_OPEN;
    return r;
  }

  pid_t main_pid = getpid();
  pid_t pid = fork();
  if (pid == -1) {
    r.status = EXEC_FORK_ERROR;
    close_io(fileIn, fileOut);
    panic("fork failed");
  }

  if (pid == 0) {
    setpgid(0, abs(main_pid)); // Set process group id to main_pid to avoid
                               // signals being sent to the shell
    const int argc = command->args.len + /*cmd*/ 1 + /*NULL*/ 1;
    char *argv[argc];
    char *cmd = slice_to_stack_str(command->name);
    argv[0] = cmd;
    for (size_t i = 0; i < command->args.len; i++) {
      argv[i + 1] = slice_to_stack_str(command->args.data[i]);
    }
    argv[argc - 1] = NULL;

    if (dup2(fileIn, STDIN_FILENO) == -1) {
      panic("dup2 failed");
    }
    if (dup2(fileOut, STDOUT_FILENO) == -1) {
      panic("dup2 failed");
    }
    close_io(fileIn, fileOut);
    execvp(cmd, argv);
    log_warn("Command not found: %s\n", cmd);
    _exit(1);
  }

  close_io(fileIn, fileOut);

  r.pid = pid;
  if (command->flags & CMD_BG || command->flags & CMD_PIPE) {
    log_info("[PID: %d] Started process with PID %d\n", main_pid, pid);
    r.status = command->flags & CMD_PIPE ? EXEC_PIPELINE : EXEC_IN_BACKGROUND;
    return r;
  }

  int status;
  do {
    if (waitpid(pid, &status, WUNTRACED) == -1) {
      panic("waitpid failed");
    }
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  r.exit_code = WEXITSTATUS(status);
  return r;
}
