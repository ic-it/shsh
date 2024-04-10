#include "parser.h"
#include "lexer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Command parse_command(Parser *parser);

// UTILITY FUNCTIONS
char *strdup(const char *s, size_t len) {
  char *d = malloc(len + 1);
  if (d == NULL)
    return NULL;
  memcpy(d, s, len);
  d[len] = '\0';
  return d;
}
//

char *get_token_value(Parser *parser, Token token) {
  return strdup(parser->input + token.position, token.length);
}

void parser_advance(Parser *parser) {
  parser->current_token = lex_next(parser->lexer);
}

Token parser_peek(Parser *parser) { return parser->current_token; }

int parser_eat(Parser *parser, TokenType type) {
  if (parser_peek(parser).type == type) {
    parser_advance(parser);
    return 1;
  }
  return 0;
}

int eat_any_word(Parser *parser) {
  return parser_eat(parser, TOKEN_WORD) ||
         parser_eat(parser, TOKEN_ESCAPED_WORD);
}

Command parse_next(Parser *parser) {
  if (parser->lexer->position == 0) {
    parser_advance(parser);
  }

  while (parser_eat(parser, TOKEN_NEWLINE) ||
         parser_eat(parser, TOKEN_SEMICOLON)) {
  }
  if (parser_peek(parser).type == TOKEN_EOF) {
    return (Command){0};
  }
  Command command = parse_command(parser);
  return command;
}

Command parse_command(Parser *parser) {
  Command command = {0};
  Token current_token = parser_peek(parser);

  if (!parser_eat(parser, TOKEN_WORD)) {
    printf("Error: expected command name\n");
    command.result = PARSE_ERROR;
    return command;
  }
  command.name = get_token_value(parser, current_token);
  current_token = parser_peek(parser);

  while (parser_eat(parser, TOKEN_WORD)) {
    char *arg = get_token_value(parser, current_token);
    command.argv = realloc(command.argv, sizeof(char *) * (command.argc + 1));
    command.argv[command.argc++] = arg;
    current_token = parser_peek(parser);
  }

  if (parser_eat(parser, TOKEN_REDIRECT_IN)) {
    current_token = parser_peek(parser);
    if (!eat_any_word(parser)) {
      printf("Error: expected file after redirect in\n");
      command.result = PARSE_ERROR;
      return command;
    }
    command.in_file = get_token_value(parser, current_token);
    command.type |= CMD_FILE_IN;
  }

  if (parser_eat(parser, TOKEN_REDIRECT_OUT)) {
    current_token = parser_peek(parser);
    if (!eat_any_word(parser)) {
      printf("Error: expected file after redirect out\n");
      command.result = PARSE_ERROR;
      return command;
    }
    command.out_file = get_token_value(parser, current_token);
    command.type |= CMD_FILE_OUT;
  }

  if (parser_eat(parser, TOKEN_REDIRECT_FROM)) {
    current_token = parser_peek(parser);
    if (!eat_any_word(parser)) {
      printf("Error: expected tcp after tcp in\n");
      command.result = PARSE_ERROR;
      return command;
    }
    command.in_tcp = get_token_value(parser, current_token);
    command.type |= CMD_TCP_IN;
  }

  if (parser_eat(parser, TOKEN_REDIRECT_AT)) {
    current_token = parser_peek(parser);
    if (!eat_any_word(parser)) {
      printf("Error: expected tcp after tcp out\n");
      command.result = PARSE_ERROR;
      return command;
    }
    command.out_tcp = get_token_value(parser, current_token);
    command.type |= CMD_TCP_OUT;
  }

  if (parser_eat(parser, TOKEN_PIPE)) {
    current_token = parser_peek(parser);
    command.type |= CMD_PIPE;
  }

  if (parser_eat(parser, TOKEN_BG)) {
    current_token = parser_peek(parser);
    command.type |= CMD_BG;
  }

  return command;
}
