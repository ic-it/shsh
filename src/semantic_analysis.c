#include "semantic_analysis.h"

const char *semantic_reasons[] = {
    "TCP IN/OUT and FILE IN/OUT cannot be used together",
    "OUT to the file/tcp and PIPE cannot be used together",
    "PIPE and BACKGROUND cannot be used together",
};

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
  if ((command->flags & CMD_PIPE) && (command->flags & CMD_BG)) {
    return (SemanticResult){.reason = REASON_BACKGROUND_CONFLICT,
                            .result = SEMANTIC_ERROR};
  }
  return (SemanticResult){.result = SEMANTIC_OK};
}

const char *get_semantic_reason(SemanticReasonEnum reason) {
  return semantic_reasons[reason];
}
