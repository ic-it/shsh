#pragma once

#include "lexer.h"
#include "types.h"

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
} ParseResultEnum;

/// @brief Command structure
typedef struct {
  Slice name;       // Command name
  SliceVec args;    // Command arguments
  Slice in_file;    // Input file
  Slice out_file;   // Output file
  Slice in_tcp;     // Input TCP
  Slice out_tcp;    // Output TCP
  CommandType type; // Command type (file, tcp, bg)
} Command;

/// @brief clear_command
/// Clear the command structure
/// @param command Command
void clear_command(Command command);

/// ParseResult
typedef struct {
  Command command;
  ParseResultEnum result;
} ParseResult;

/// @brief Command Parser
/// Iterator over Commands
typedef struct {
  Lexer *lexer;
  Token current_token;
  char *input;
} Parser;

/// @brief Iterate over the commands in the input
/// @param parser Parser
/// @return ParseResult
ParseResult parse_next(Parser *parser);
