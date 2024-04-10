#pragma once

#include "parser.h"

typedef enum {
  SEMANTIC_OK,
  SEMANTIC_ERROR,
  SEMANTIC_EOF,
} SemanticResultEnum;

const char *error_reasons[] = {
    "TCP IN/OUT and FILE IN/OUT cannot be used together",
    "OUT to the file/tcp and PIPE cannot be used together",
};

typedef enum {
  REASON_IO_CONFLICT,
  REASON_PIPE_CONFLICT,
} ReasonEnum;

typedef struct {
  ReasonEnum reason;
  SemanticResultEnum result;
} SemanticResult;

/// @brief Analyze next command
/// @param Command command
/// @return SemanticResult
SemanticResult semantic_analyze(Command *command) {
  if ((command->flags & CMD_TCP_IN) && (command->flags & CMD_FILE_IN)) {
    return (SemanticResult){.reason = REASON_IO_CONFLICT,
                            .result = SEMANTIC_ERROR};
  }
  if ((command->flags & CMD_TCP_OUT) && (command->flags & CMD_FILE_OUT)) {
    return (SemanticResult){.reason = REASON_IO_CONFLICT,
                            .result = SEMANTIC_ERROR};
  }
  if ((command->flags & (CMD_TCP_OUT | CMD_FILE_OUT)) &&
      (command->flags & CMD_PIPE)) {
    return (SemanticResult){.reason = REASON_PIPE_CONFLICT,
                            .result = SEMANTIC_ERROR};
  }
  return (SemanticResult){.result = SEMANTIC_OK};
}
