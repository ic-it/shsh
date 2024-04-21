#pragma once

#include "parser.h"
#include "semantic_analysis.h"
#include "types.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum {
  EXEC_SUCCESS,
  EXEC_PARSE_ERROR,
  EXEC_SEMANTIC_ERROR,
  EXEC_PARSE_EOF,
  EXEC_IN_BACKGROUND,
  EXEC_PIPELINE,
  EXEC_ERROR_FILE_OPEN,
} ExecStatusEnum;

typedef struct {
  SemanticReasonEnum semantic_reason;
  ExecStatusEnum status;
  int exit_code;
  bool is_background;
  bool is_pipeline;
} ExecResult;

typedef struct {
  pid_t *pids;
  size_t pids_size;
  size_t pids_capacity;

  pthread_mutex_t mutex;
} Jobs;

/// @brief Create a new jobs struct
/// @note Allocates memory, so you must call jobs_free when done
Jobs *jobs_new(void);

/// @brief Free a jobs struct
void jobs_free(Jobs *jobs);

/// @brief Push a pid to the jobs list
/// @param jobs Jobs struct
/// @param pid Process ID
/// @param next If true, push to the end of the list
size_t push_pid(Jobs *jobs, int pid, bool next);

/// @brief Remove a pid from the jobs list
/// @param jobs Jobs struct
/// @param pid Process ID
void remove_pid(Jobs *jobs, int pid);

/// @brief Executor struct
typedef struct {
  Parser *parser;
  Jobs *jobs;
} Executor;

/// @brief Create a new executor
/// @note Allocates memory, so you must call executor_free when done
Executor executor_new(Parser *parser, Jobs *jobs);

/// @brief Execute the next command or pipeline
ExecResult exec_next(Executor *executor, int in_fd, int out_fd);
