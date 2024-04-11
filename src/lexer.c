#include "lexer.h"
#include "types.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char lexer_peek(Lexer *lexer) { return lexer->input[lexer->position]; }

char lexer_lookahead(Lexer *lexer, int n) {
  return lexer->input[lexer->position + n];
}

char lexer_advance(Lexer *lexer) {
  char c = lexer_peek(lexer);
  if (c != '\0')
    lexer->position++;
  return c;
}

int lexer_eat(Lexer *lexer, char c) {
  if (lexer_peek(lexer) == c) {
    lexer_advance(lexer);
    return 1;
  }
  return 0;
}

int isescaped(int c) { return c != '\''; }

int isnonspecial(int c) {
  return !isspace(c) && c != ';' && c != '|' && c != '>' && c != '<' &&
         c != '\'' && c != '&' && c != '\0' && c != '#';
}

Lexer lex_new(char *input) { return (Lexer){.input = input, .position = 0}; }

void lex_reset(Lexer *lexer, char *input) {
  lexer->input = input;
  lexer->position = 0;
}

Token lex_next(Lexer *lexer) {
  int current_position = lexer->position;

  // Skip whitespace
  while ((lexer_peek(lexer) == ' ') || (lexer_peek(lexer) == '\t') ||
         (lexer_peek(lexer) == '\r')) {
    lexer_advance(lexer);
    current_position++;
  }

  // Skip comments
  if (lexer_peek(lexer) == '#') {
    while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') {
      lexer_advance(lexer);
      current_position++;
    }
  }

  // Check for end of input
  if (lexer_eat(lexer, '\0'))
    return (Token){.type = TOKEN_EOF, .value = (Slice){0}};

  // Check for unescaped word (\<char> | <char>)+
  if (lexer_peek(lexer) == '\\' || isnonspecial(lexer_peek(lexer))) {
    int state = 0; // 0: normal, 1: escaped
    do {
      if (lexer_peek(lexer) == '\0') {
        if (state)
          return (Token){.type = TOKEN_ERROR,
                         .value = (Slice){
                             .data = lexer->input,
                             .pos = current_position,
                             .len = lexer->position - current_position,
                         }};
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
    } while (state || isnonspecial(lexer_peek(lexer)));

    return (Token){.type = TOKEN_WORD,
                   .value = (Slice){
                       .data = lexer->input,
                       .pos = current_position,
                       .len = lexer->position - current_position,
                   }};
  }

  // Check for escaped word
  if (lexer_eat(lexer, '\'')) {
    int state = 0; // 0: normal, 1: escaped
    while (isescaped(lexer_peek(lexer)) || state) {
      if (lexer_peek(lexer) == '\0') {
        return (Token){
            .type = TOKEN_ERROR,
            .value =
                (Slice){
                    .data = lexer->input,
                    .pos = current_position,
                    .len = lexer->position - current_position,
                },
        };
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
      return (Token){.type = TOKEN_ERROR,
                     .value = (Slice){
                         .data = lexer->input,
                         .pos = current_position,
                         .len = lexer->position - current_position,
                     }};
    return (Token){.type = TOKEN_ESCAPED_WORD,
                   .value = (Slice){
                       .data = lexer->input,
                       .pos = current_position + 1,
                       .len = lexer->position - current_position - 2,
                   }};
  }

  // Check for special characters
  TokenType type = TOKEN_ERROR;
  int length = 1;

  switch (lexer_advance(lexer)) {
  case '|':
    type = TOKEN_PIPE;
    break;
  case '&':
    type = TOKEN_BG;
    break;
  case '>':
    if (lexer_eat(lexer, '@')) {
      type = TOKEN_TCP_OUT;
      length = 2;
      break;
    }
    type = TOKEN_FILE_OUT;
    break;
  case '<':
    if (lexer_eat(lexer, '@')) {
      type = TOKEN_TCP_IN;
      length = 2;
      break;
    }
    type = TOKEN_FILE_IN;
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
  return (Token){.type = type,
                 .value = (Slice){
                     .data = lexer->input,
                     .pos = current_position,
                     .len = length,
                 }};
}
