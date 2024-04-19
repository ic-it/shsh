#pragma once

#include "parser.h"
#include <sys/types.h>

typedef enum {
  EXEC_SUCCESS,
  EXEC_IN_BACKGROUND,
  EXEC_FORK_ERROR,
  EXEC_ERROR_FILE_OPEN,
  EXEC_WAIT_ERROR,
} ExecStatusEnum;

// typedef struct {
// } Executor;

typedef struct {
  ExecStatusEnum status;
  pid_t pid;
  int exit_code;
} ExecResult;

/// @brief Execute single command
ExecResult exec_command(Command *command);
