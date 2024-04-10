#pragma once
#include "types.h"

typedef enum {
  TOKEN_WORD,         // word without special characters
  TOKEN_ESCAPED_WORD, // '<any chars except \'>'
  TOKEN_PIPE,         // |
  TOKEN_BG,           // &
  TOKEN_FILE_OUT,     // >
  TOKEN_FILE_IN,      // <
  TOKEN_TCP_OUT,      // >@
  TOKEN_TCP_IN,       // <@
  TOKEN_SEMICOLON,    // ;
  TOKEN_NEWLINE,      // \n
  TOKEN_EOF,          // end of file
  TOKEN_ERROR         // error
} TokenType;

/// @brief Token structure
typedef struct {
  TokenType type;
  Slice value;
} Token;

/// @brief Program Lexer
/// Initial position is 0
typedef struct {
  char *input;
  int position;
} Lexer;

/// @brief Iterator over Tokens
/// @param lexer
/// @return Token
Token lex_next(Lexer *lexer);
