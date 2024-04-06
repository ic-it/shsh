#pragma once

typedef enum {
  TOKEN_WORD,          // word without special characters
  TOKEN_ESCAPED_WORD,  // '<any chars except \'>'
  TOKEN_ADDRESS,       // <ip>:<port>
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
  char *value;
} Token;

typedef struct {
  char *input;
  int position;
} Lexer;

Lexer *init_lexer(char *input);
Token *lexer_next_token(Lexer *lexer);