#pragma once

#include "parser.h"

typedef enum {
  SEMANTIC_OK,
  SEMANTIC_ERROR,
  SEMANTIC_EOF,
} SemanticResultEnum;

typedef enum {
  REASON_IO_CONFLICT,
  REASON_PIPE_CONFLICT,
  REASON_BACKGROUND_CONFLICT,
} SemanticReasonEnum;

typedef struct {
  SemanticReasonEnum reason;
  SemanticResultEnum result;
} SemanticResult;

/// @brief Analyze next command
/// @param Command command
/// @return SemanticResult
SemanticResult semantic_analyze(Command *command);

/// @brief Get semantic reason
/// @param SemanticReasonEnum reason
/// @return const char*
const char *get_semantic_reason(SemanticReasonEnum reason);
