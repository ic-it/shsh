#pragma once
#include "log.h"
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_STACK_FRAMES 64

#define panic(message)                                                         \
  {                                                                            \
    log_error("Panic: %s\n", message);                                         \
    void *stackFrames[MAX_STACK_FRAMES];                                       \
    int numFrames;                                                             \
    numFrames = backtrace(stackFrames, MAX_STACK_FRAMES);                      \
    backtrace_symbols_fd(stackFrames, numFrames, STDERR_FILENO);               \
    exit(EXIT_FAILURE);                                                        \
  }

#define panicf(fmt, ...)                                                       \
  {                                                                            \
    char buf[1024];                                                            \
    snprintf(buf, sizeof(buf), fmt, __VA_ARGS__);                              \
    panic(buf);                                                                \
  }

#define todo() panic("TODO")
#define unreachable() panic("Unreachable code reached")
#define not_implemented() panic("Not implemented")
#define assert(cond)                                                           \
  {                                                                            \
    if (!(cond)) {                                                             \
      panic("Assertion failed: " #cond);                                       \
    }                                                                          \
  }
#define assertf(cond, fmt, ...)                                                \
  {                                                                            \
    if (!(cond)) {                                                             \
      panicf("Assertion failed: " fmt, __VA_ARGS__);                           \
    }                                                                          \
  }
