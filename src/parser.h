#pragma once

typedef enum {
  TOKEN_WORD,          // word without special characters
  TOKEN_ESCAPED_WORD,  // '<any chars except \'>'
  TOKEN_PIPE,          // |
  TOKEN_BG,            // &
  TOKEN_REDIRECT_OUT,  // >
  TOKEN_REDIRECT_IN,   // <
  TOKEN_REDIRECT_AT,   // >@
  TOKEN_REDIRECT_FROM, // <@
  TOKEN_SEMICOLON,     // ;
  TOKEN_NEWLINE,       // \n
  TOKEN_EOF,           // end of file
  TOKEN_ERROR          // error
} TokenType;

typedef struct {
  TokenType type;
  int position;
  int length;
} Token;

typedef struct {
  const char *input;
  int position;
} Lexer;

Token lexer_next_token(Lexer *lexer);
