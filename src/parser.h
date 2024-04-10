#pragma once

#include "lexer.h"

typedef enum {
  CMD_FILE_IN = 1,  // Command reads from file
  CMD_FILE_OUT = 2, // Command writes to file
  CMD_TCP_IN = 4,   // Command reads from TCP
  CMD_TCP_OUT = 8,  // Command writes to TCP
  CMD_BG = 16,      // Command is background
  CMD_PIPE = 32,    // Command is piped
} CommandType;

/// Parsing Result
typedef enum {
  PARSE_OK = 0, // Parsing was successful
  PARSE_ERROR,  // Parsing failed
  PARSE_EOF,    // End of file
} ParseResult;

/// @brief Command structure
typedef struct {
  char *name;       // Command name
  char **argv;      // Arguments
  int argc;         // Number of arguments
  char *in_file;    // Input file
  char *out_file;   // Output file
  char *in_tcp;     // Input TCP
  char *out_tcp;    // Output TCP
  CommandType type; // Command type (file, tcp, bg)
  ParseResult result;
} Command;

/// @brief Command Parser
/// Iterator over Commands
typedef struct {
  Lexer *lexer;
  Token current_token;
  char *input;
} Parser;

/// @brief Iterate over the commands in the input
/// @param parser Parser
/// @return Command
Command parse_next(Parser *parser);
