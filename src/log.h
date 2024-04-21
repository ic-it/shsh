#pragma once

#include <stdio.h>
#include <string.h>

// Color codes
#define _LOG_CLR_RESET "\x1b[0m"
#define _LOG_CLR_RED "\x1b[31m"
#define _LOG_CLR_GREEN "\x1b[32m"
#define _LOG_CLR_YELLOW "\x1b[33m"
#define _LOG_CLR_BLUE "\x1b[34m"
#define _LOG_CLR_MAGENTA "\x1b[35m"
#define _LOG_CLR_CYAN "\x1b[36m"
#define _LOG_CLR_WHITE "\x1b[37m"

#define _LOG_CLR_BOLD "\x1b[1m"
#define _LOG_CLR_DIM "\x1b[2m"
#define _LOG_CLR_UNDERLINE "\x1b[4m"
#define _LOG_CLR_BLINK "\x1b[5m"
#define _LOG_CLR_REVERSE "\x1b[7m"
#define _LOG_CLR_HIDDEN "\x1b[8m"

// Log levels
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3

// Log level colors
#define _LOG_CLR_LOG_LEVEL_DEBUG _LOG_CLR_CYAN
#define _LOG_CLR_LOG_LEVEL_INFO _LOG_CLR_GREEN
#define _LOG_CLR_LOG_LEVEL_WARN _LOG_CLR_YELLOW
#define _LOG_CLR_LOG_LEVEL_ERROR _LOG_CLR_RED

// Log level names
#define _LOG_NAME_LOG_LEVEL_DEBUG "DEBUG"
#define _LOG_NAME_LOG_LEVEL_INFO "INFO"
#define _LOG_NAME_LOG_LEVEL_WARN "WARN"
#define _LOG_NAME_LOG_LEVEL_ERROR "ERROR"

// Default log level
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Log function
#define log(file, level, fmt, ...)                                             \
  {                                                                            \
    if (level >= LOG_LEVEL) {                                                  \
      fprintf(file,                                                            \
              _LOG_CLR_##level _LOG_CLR_BOLD _LOG_CLR_REVERSE                  \
              "%s" _LOG_CLR_RESET "\t| " _LOG_CLR_##level _LOG_CLR_BOLD        \
              "%s:%d" _LOG_CLR_RESET " | " fmt,                                \
              _LOG_NAME_##level, __FILE__, __LINE__, __VA_ARGS__);             \
    }                                                                          \
  }

// Log into specified fd
#define log_debug_fd(file, fmt, ...)                                           \
  log(file, LOG_LEVEL_DEBUG, fmt, __VA_ARGS__)
#define log_info_fd(file, fmt, ...) log(file, LOG_LEVEL_INFO, fmt, __VA_ARGS__)
#define log_warn_fd(file, fmt, ...) log(file, LOG_LEVEL_WARN, fmt, __VA_ARGS__)
#define log_error_fd(file, fmt, ...)                                           \
  log(file, LOG_LEVEL_ERROR, fmt, __VA_ARGS__)

// Log functions
#define log_debug(fmt, ...) log_debug_fd(stdout, fmt, __VA_ARGS__)
#define log_info(fmt, ...) log_info_fd(stdout, fmt, __VA_ARGS__)
#define log_warn(fmt, ...) log_warn_fd(stdout, fmt, __VA_ARGS__)
#define log_error(fmt, ...) log_error_fd(stderr, fmt, __VA_ARGS__)
