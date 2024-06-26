#include "parser.h"
#include "lexer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// @brief Parse next command
/// @return ParseResult
ParseResult parse_command(Parser *parser);

/// @brief Parse a command body
/// @return is ok
int parse_command_body(Parser *parser, Command *command);

/// @brief Parse command name
/// @return is ok
int parse_command_name(Parser *parser, Command *command);

/// @brief Parse arguments
/// @return is ok
int parse_arguments(Parser *parser, Command *command);

/// @brief Parse io redirection (file, tcp)
/// @return is ok
int parse_io(Parser *parser, Command *command);

void parser_advance(Parser *parser) {
  parser->current_token = lex_next(parser->lexer);
}

Token parser_peek(Parser *parser) { return parser->current_token; }

int parser_match(Parser *parser, TokenType type) {
  if (parser_peek(parser).type == type) {
    return 1;
  }
  return 0;
}

int parser_eat(Parser *parser, TokenType type) {
  if (parser_peek(parser).type == type) {
    parser_advance(parser);
    return 1;
  }
  return 0;
}

int parser_match_any_word(Parser *parser) {
  return parser_match(parser, TOKEN_WORD) ||
         parser_match(parser, TOKEN_ESCAPED_WORD);
}

int parser_eat_any_word(Parser *parser) {
  return parser_eat(parser, TOKEN_WORD) ||
         parser_eat(parser, TOKEN_ESCAPED_WORD);
}

void clear_command_args(Command command) { slice_vec_free(&command.args); }

Parser parse_new(Lexer *lexer) {
  return (Parser){
      .lexer = lexer,
      .current_token = lex_next(lexer),
  };
}

ParseResult parse_next(Parser *parser) {
  while (parser_eat(parser, TOKEN_NEWLINE) ||
         parser_eat(parser, TOKEN_SEMICOLON)) {
  }
  if (parser_match(parser, TOKEN_EOF)) {
    return (ParseResult){
        .command = (Command){0},
        .result = PARSE_EOF,
    };
  }
  ParseResult pr = parse_command(parser);

  if (parser_match(parser, TOKEN_ERROR)) {
    pr.result = PARSE_ERROR;
  }

  if (pr.result == PARSE_ERROR) {
    while (!parser_eat(parser, TOKEN_NEWLINE) &&
           !parser_eat(parser, TOKEN_EOF)) {
      parser_advance(parser);
    }
  }

  return pr;
}

ParseResult parse_command(Parser *parser) {
  ParseResult r = {
      .command =
          (Command){
              .args = slice_vec_new(),
          },
      .result = PARSE_OK,
  };

  if (parser_match_any_word(parser)) {
    if (!parse_command_body(parser, &r.command)) {
      r.result = PARSE_ERROR;
      return r;
    }
  } else {
    r.result = PARSE_ERROR;
    return r;
  }

  if (parser_eat(parser, TOKEN_BG)) {
    r.command.flags |= CMD_BG;
  }
  if (parser_eat(parser, TOKEN_PIPE)) {
    r.command.flags |= CMD_PIPE;
  }

  return r;
}

int parse_command_body(Parser *parser, Command *command) {
  if (!parse_command_name(parser, command)) {
    return 0;
  }
  if (!parse_arguments(parser, command)) {
    return 0;
  }
  if (!parse_io(parser, command)) {
    return 0;
  }
  return 1;
}

int parse_command_name(Parser *parser, Command *command) {
  if (parser_match_any_word(parser)) {
    command->name = parser_peek(parser).value;
    parser_advance(parser);
    return 1;
  }
  return 0;
}

int parse_arguments(Parser *parser, Command *command) {
  while (parser_match_any_word(parser)) {
    slice_vec_push(&command->args, parser_peek(parser).value);
    parser_advance(parser);
  }
  return 1;
}

int parse_io(Parser *parser, Command *command) {
  struct {
    TokenType type;
    int flag;
    Slice *value;
  } io[] = {
      {TOKEN_FILE_OUT, CMD_FILE_OUT, &command->out_file},
      {TOKEN_FILE_IN, CMD_FILE_IN, &command->in_file},
      {TOKEN_TCP_OUT, CMD_TCP_OUT, &command->out_tcp},
      {TOKEN_TCP_IN, CMD_TCP_IN, &command->in_tcp},
  };
  // j -- is a counter of iterations
  // i -- is an index of io array
  for (size_t j = 0, i = 0; j < 8; j++, i = (i + 1) % 4) {
    if (parser_eat(parser, io[i].type)) {
      if (parser_match_any_word(parser)) {
        command->flags |= io[i].flag;
        slice_assign(io[i].value, parser_peek(parser).value);
        parser_advance(parser);
        i = 0;
      } else {
        return 0;
      }
    }
  }
  return 1;
}
