#include "parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *init_token(TokenType type, char *value) {
  Token *token = malloc(sizeof(Token));
  token->type = type;
  token->value = value;
  return token;
}

Lexer *init_lexer(char *input) {
  Lexer *lexer = malloc(sizeof(Lexer));
  lexer->input = input;
  lexer->position = 0;
  return lexer;
}

// Advance lexer position
void lexer_advance(Lexer *lexer) {
  if (lexer->input[lexer->position] != '\0')
    lexer->position++;
}

// Read characters until a specific condition is met
char *lexer_collect(Lexer *lexer, int (*condition)(int)) {
  char *value = malloc(1);
  int value_size = 0;

  while (condition(lexer->input[lexer->position])) {
    value = realloc(value, ++value_size);
    value[value_size - 1] = lexer->input[lexer->position];
    lexer_advance(lexer);
  }

  value = realloc(value, ++value_size);
  value[value_size - 1] = '\0';
  return value;
}

// Lexer Rollback
void lexer_rollback(Lexer *lexer, int n) {
  lexer->position -= n;
  if (lexer->position < 0)
    lexer->position = 0;
}

// Collect escaped word
int isescaped(int c) { return c != '\''; }

// Collect unescaped word
int isnonspecial(int c) {
  return !isspace(c) && c != ';' && c != '|' && c != '>' && c != '<' &&
         c != '*' && c != '\'';
}

// Collect Seems like a valid address
int isaddress(int c) { return isdigit(c) || c == '.'; }

// Get next token from lexer
Token *lexer_next_token(Lexer *lexer) {
  char current_char = lexer->input[lexer->position];

  if (current_char == '\0')
    return init_token(TOKEN_EOF, NULL);

  // Skip whitespace
  while (isspace(current_char)) {
    lexer_advance(lexer);
    current_char = lexer->input[lexer->position];
  }

  // Check for end of input
  if (current_char == '\0')
    return init_token(TOKEN_EOF, NULL);

  // Check for address (word or <ip>:<port>)
  if (isdigit(current_char)) {
    char *value = lexer_collect(lexer, isaddress);
    if (lexer->input[lexer->position] == ':') {
      lexer_advance(lexer);
      char *port = lexer_collect(lexer, isdigit);
      char *address = malloc(strlen(value) + strlen(port) + 2);
      sprintf(address, "%s:%s", value, port);
      free(value);
      free(port);
      return init_token(TOKEN_ADDRESS, address);
    }
    return init_token(TOKEN_WORD, value);
  }

  // Check for command word
  if (isnonspecial(current_char)) {
    char *value = lexer_collect(lexer, isnonspecial);
    return init_token(TOKEN_WORD, value);
  }

  // Check for escaped word
  if (current_char == '\'') {
    lexer_advance(lexer);
    char *value = lexer_collect(lexer, isescaped);
    lexer_advance(lexer);
    return init_token(TOKEN_ESCAPED_WORD, value);
  }

  // Check for special characters
  switch (current_char) {
  case '|':
    lexer_advance(lexer);
    return init_token(TOKEN_PIPE, NULL);
  case '&':
    lexer_advance(lexer);
    return init_token(TOKEN_BG, NULL);
  case '>':
    lexer_advance(lexer);
    if (lexer->input[lexer->position] == '@') {
      lexer_advance(lexer);
      return init_token(TOKEN_REDIRECT_AT, NULL);
    }
    return init_token(TOKEN_REDIRECT_OUT, NULL);
  case '<':
    lexer_advance(lexer);
    if (lexer->input[lexer->position] == '@') {
      lexer_advance(lexer);
      return init_token(TOKEN_REDIRECT_FROM, NULL);
    }
    return init_token(TOKEN_REDIRECT_IN, NULL);
  case ';':
    lexer_advance(lexer);
    return init_token(TOKEN_SEMICOLON, NULL);
  case '\n':
    lexer_advance(lexer);
    return init_token(TOKEN_NEWLINE, NULL);
  default:
    return init_token(TOKEN_ERROR, NULL);
  }
}
