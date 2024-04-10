#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Peek next character from lexer
char lexer_peek(Lexer *lexer) { return lexer->input[lexer->position]; }

/// Lookahead next character from lexer
char lexer_lookahead(Lexer *lexer, int n) {
  return lexer->input[lexer->position + n];
}

/// Advance lexer position
char lexer_advance(Lexer *lexer) {
  char c = lexer_peek(lexer);
  if (c != '\0')
    lexer->position++;
  return c;
}

/// Eat next character from lexer if it matches the given character
int lexer_eat(Lexer *lexer, char c) {
  if (lexer_peek(lexer) == c) {
    lexer_advance(lexer);
    return 1;
  }
  return 0;
}

/// Collect escaped word
int isescaped(int c) { return c != '\''; }

/// Collect unescaped word
int isnonspecial(int c) {
  return !isspace(c) && c != ';' && c != '|' && c != '>' && c != '<' &&
         c != '*' && c != '\'' && c != '&' && c != '\0';
}

/// Get next token from lexer
Token lex_next(Lexer *lexer) {
  int current_position = lexer->position;

  // Skip whitespace
  while (isspace(lexer_peek(lexer))) {
    lexer_advance(lexer);
    current_position++;
  }

  // Check for end of input
  if (lexer_eat(lexer, '\0'))
    return (Token){.type = TOKEN_EOF, .position = lexer->position, .length = 0};

  // Check for unescaped word (\<char> | <char>)+
  if (lexer_peek(lexer) == '\\' || isnonspecial(lexer_peek(lexer))) {
    int state = 0; // 0: normal, 1: escaped
    while (isnonspecial(lexer_peek(lexer)) || state) {
      if (lexer_peek(lexer) == '\0') {
        if (state)
          return (Token){.type = TOKEN_ERROR};
        break;
      }
      if (lexer_peek(lexer) == '\\') {
        state = !state;
        // FIXME: SHITTY CODE! I'm too lazy to fix this
        memmove((lexer->input + lexer->position),
                (lexer->input + lexer->position + 1),
                strlen(lexer->input + lexer->position + 1));
        lexer->input[strlen(lexer->input) - 1] = '\0';
        continue;
      } else {
        state = 0;
      }
      lexer_advance(lexer);
    }

    return (Token){
        .type = TOKEN_WORD,
        .position = current_position,
        .length = lexer->position - current_position,
    };
  }

  // Check for escaped word
  if (lexer_eat(lexer, '\'')) {
    int state = 0; // 0: normal, 1: escaped
    while (isescaped(lexer_peek(lexer)) || state) {
      if (lexer_peek(lexer) == '\0') {
        return (Token){.type = TOKEN_ERROR};
      }
      if (lexer_peek(lexer) == '\\') {
        // FIXME: SHITTY CODE! I'm too lazy to fix this
        state = !state;
        memmove((lexer->input + lexer->position),
                (lexer->input + lexer->position + 1),
                strlen(lexer->input + lexer->position + 1));
        lexer->input[strlen(lexer->input) - 1] = '\0';
        continue;
      } else {
        state = 0;
      }
      lexer_advance(lexer);
    }

    if (!lexer_eat(lexer, '\''))
      return (Token){
          .type = TOKEN_ERROR,
          .position = current_position,
          .length = lexer->position - current_position,
      };
    return (Token){
        .type = TOKEN_ESCAPED_WORD,
        .position = current_position + 1,
        .length = lexer->position - current_position - 2,
    };
  }

  // Check for special characters
  TokenType type = TOKEN_ERROR;
  int length = 1;

  switch (lexer_advance(lexer)) {
  case '*':
    type = TOKEN_STAR;
    break;
  case '|':
    type = TOKEN_PIPE;
    break;
  case '&':
    type = TOKEN_BG;
    break;
  case '>':
    if (lexer_eat(lexer, '@')) {
      type = TOKEN_REDIRECT_AT;
      length = 2;
      break;
    }
    type = TOKEN_REDIRECT_OUT;
    break;
  case '<':
    if (lexer_eat(lexer, '@')) {
      type = TOKEN_REDIRECT_FROM;
      length = 2;
      break;
    }
    type = TOKEN_REDIRECT_IN;
    break;
  case ';':
    type = TOKEN_SEMICOLON;
    break;
  case '\n':
    type = TOKEN_NEWLINE;
    break;
  default:
    break;
  }
  return (Token){
      .type = type,
      .position = current_position,
      .length = length,
  };
}
