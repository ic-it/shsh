#pragma once

#include "parser.h"
#include "types.h"
#include <sys/types.h>

typedef enum {
  EXEC_SUCCESS,
  EXEC_IN_BACKGROUND,
  EXEC_PIPELINE,
  EXEC_FORK_ERROR,
  EXEC_PIPE_ERROR,
  EXEC_ERROR_FILE_OPEN,
  EXEC_WAIT_ERROR,
} ExecStatusEnum;

typedef struct {
  ExecStatusEnum status;
  pid_t pid;
  int exit_code;
  int pipe;
} ExecResult;

/// @brief Execute single command
ExecResult exec_command(Command *command, int pipe_in);
