#include "exec.h"
#include "parser.h"
#include "types.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define clear_argv(argc, argv)                                                 \
  {                                                                            \
    for (int i = 0; i < argc; i++)                                             \
      free(argv[i]);                                                           \
  }

int open_io(Command *command, int *fileIn, int *fileOut) {
  if (command->flags & CMD_FILE_IN) {
    char *file = slice_to_str(command->in_file);
    *fileIn = open(file, O_RDONLY);
    free(file);
    if (*fileIn == -1) {
      return -1;
    }
  }

  if (command->flags & CMD_FILE_OUT) {
    char *file = slice_to_str(command->out_file);
    *fileOut = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    free(file);
    if (*fileOut == -1) {
      return -1;
    }
  }

  return 0;
}

void close_io(Command *command, int fileIn, int fileOut) {
  if (command->flags & CMD_FILE_IN && fileIn != STDIN_FILENO) {
    close(fileIn);
  }

  if (command->flags & CMD_FILE_OUT && fileOut != STDOUT_FILENO) {
    close(fileOut);
  }
}

ExecResult exec_command(Command *command) {
  ExecResult r = {.status = EXEC_SUCCESS, .pid = 0, .exit_code = 0};
  int fileIn = STDIN_FILENO;
  int fileOut = STDOUT_FILENO;

  if (open_io(command, &fileIn, &fileOut) == -1) {
    r.status = EXEC_ERROR_FILE_OPEN;
    return r;
  }

  const int argc = command->args.len + /*cmd*/ 1 + /*NULL*/ 1;
  char *argv[argc];
  char *cmd = slice_to_str(command->name);
  argv[0] = cmd;
  for (int i = 0; i < command->args.len; i++) {
    argv[i + 1] = slice_to_str(command->args.data[i]);
  }
  argv[argc - 1] = NULL;

  pid_t main_pid = getpid();
  pid_t pid = fork();
  if (pid == -1) {
    r.status = EXEC_FORK_ERROR;
    clear_argv(argc, argv);
    close_io(command, fileIn, fileOut);
    return r;
  }

  if (pid == 0) {
    if (command->flags & CMD_BG) {
      setpgid(0, abs(main_pid));
    } else {
      setpgid(0, 0);
    }

    if (dup2(fileIn, STDIN_FILENO) == -1) {
      exit(1);
    }
    if (dup2(fileOut, STDOUT_FILENO) == -1) {
      exit(1);
    }
    execvp(cmd, argv);
    execvp("echo", (char *[]){"echo", "Command not found", NULL});
    exit(1);
  }

  r.pid = pid;
  if (command->flags & CMD_BG) {
    r.exit_code = -1;
    printf("Started process with PID %d\n", pid);
  } else {
    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1) {
      r.status = EXEC_WAIT_ERROR;
      clear_argv(argc, argv);
      close_io(command, fileIn, fileOut);
      return r;
    }
    r.exit_code = WEXITSTATUS(status);
    clear_argv(argc, argv);
    close_io(command, fileIn, fileOut);
  }

  // clear_argv(argc, argv);
  // close_io(command, fileIn, fileOut);
  return r;
}
