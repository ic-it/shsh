#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Token types
typedef enum {
  TOKEN_COMMAND_WORD,
  TOKEN_ARGUMENT,
  TOKEN_FILENAME,
  TOKEN_COMMAND_SEPARATOR,
  TOKEN_PIPE,
  TOKEN_REDIRECTION_OUTPUT,
  TOKEN_REDIRECTION_INPUT,
  TOKEN_REDIRECT_AT,
  TOKEN_DSTADDR_IP,
  TOKEN_DSTADDR_PORT,
  TOKEN_NEWLINE,
  TOKEN_EOF,
  TOKEN_ERROR
} TokenType;

// Token structure
typedef struct {
  TokenType type;
  char *value;
} Token;

// Lexer structure
typedef struct {
  char *input;
  int position;
} Lexer;

// Initialize lexer
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

// Skip whitespace
void lexer_skip_whitespace(Lexer *lexer) {
  while (isspace(lexer->input[lexer->position]))
    lexer_advance(lexer);
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

// TMP
int isescaped(int c) { return c != '\''; }
int isnonspecial(int c) {
  return !isspace(c) && c != ';' && c != '|' && c != '>' && c != '<' &&
         c != '*' && c != '\'';
}

// Get next token from lexer
Token lex_next(Lexer *lexer) {
  char current_char = lexer->input[lexer->position];

  // Skip whitespace
  while (isspace(current_char)) {
    lexer_advance(lexer);
    current_char = lexer->input[lexer->position];
  }

  // Check for end of input
  if (current_char == '\0')
    return (Token){TOKEN_EOF, NULL};

  // Check for command word
  if (isnonspecial(current_char)) {
    char *value = lexer_collect(lexer, isnonspecial);
    return (Token){TOKEN_COMMAND_WORD, value};
  }

  // Check for special characters
  switch (current_char) {
  case ';':
    lexer_advance(lexer);
    return (Token){TOKEN_COMMAND_SEPARATOR, ";"};
  case '|':
    lexer_advance(lexer);
    return (Token){TOKEN_PIPE, "|"};
  case '>':
    if (lexer->input[lexer->position + 1] == '@') {
      lexer_advance(lexer); // Skip '@'
      lexer_advance(lexer); // Move to next character
      return (Token){TOKEN_REDIRECT_AT, ">@"};
    } else {
      lexer_advance(lexer);
      return (Token){TOKEN_REDIRECTION_OUTPUT, ">"};
    }
  case '<':
    if (lexer->input[lexer->position + 1] == '@') {
      lexer_advance(lexer); // Skip '@'
      lexer_advance(lexer); // Move to next character
      return (Token){TOKEN_REDIRECT_AT, "<@"};
    } else {
      lexer_advance(lexer);
      return (Token){TOKEN_REDIRECTION_INPUT, "<"};
    }
  case '*':
    lexer_advance(lexer);
    return (Token){TOKEN_FILENAME, "*"};
  case '\'':
    lexer_advance(lexer); // Skip opening quote
    char *value = lexer_collect(lexer, isescaped);
    lexer_advance(lexer); // Skip closing quote
    return (Token){TOKEN_FILENAME, value};
  default:
    return (Token){TOKEN_ERROR, NULL};
  }
}

// Parser structure
typedef struct {
  Lexer *lexer;
  Token current_token;
} Parser;

// Initialize parser
Parser *init_parser(Lexer *lexer) {
  Parser *parser = malloc(sizeof(Parser));
  parser->lexer = lexer;
  parser->current_token = lex_next(lexer);
  return parser;
}

// Consume the current token and move to the next one
void parser_eat(Parser *parser, TokenType type) {
  printf("Eating token: %s \t| Type: %d\n", parser->current_token.value,
         parser->current_token.type); // FIXME: Remove
  if (parser->current_token.type == type)
    parser->current_token = lex_next(parser->lexer);
  else
    printf("Unexpected token type\n");
}

// Parse a program
void program(Parser *parser);

// Parse a command
void command(Parser *parser);

// Parse arguments
void arguments(Parser *parser);

// Parse a single argument
void argument(Parser *parser);

// Parse filename
void filename(Parser *parser);

// Parse destination address
void dstaddr(Parser *parser);

// Parse IP address
void ip(Parser *parser);

// Parse port
void port(Parser *parser);

// Parse a program
void program(Parser *parser) {
  command(parser);
  if (parser->current_token.type == TOKEN_COMMAND_SEPARATOR) {
    parser_eat(parser, TOKEN_COMMAND_SEPARATOR);
    program(parser);
  } else if (parser->current_token.type == TOKEN_PIPE) {
    parser_eat(parser, TOKEN_PIPE);
    program(parser);
  } else if (parser->current_token.type == TOKEN_REDIRECTION_OUTPUT ||
             parser->current_token.type == TOKEN_REDIRECTION_INPUT) {
    parser_eat(parser, parser->current_token.type);
    filename(parser);
    if (parser->current_token.type == TOKEN_COMMAND_SEPARATOR)
      program(parser);
  } else if (parser->current_token.type == TOKEN_REDIRECT_AT) {
    parser_eat(parser, TOKEN_REDIRECT_AT);
    dstaddr(parser);
    if (parser->current_token.type == TOKEN_COMMAND_SEPARATOR)
      program(parser);
  }
}

// Parse a command
void command(Parser *parser) {
  parser_eat(parser, TOKEN_COMMAND_WORD);
  if (parser->current_token.type != TOKEN_EOF &&
      parser->current_token.type != TOKEN_COMMAND_SEPARATOR &&
      parser->current_token.type != TOKEN_PIPE &&
      parser->current_token.type != TOKEN_REDIRECTION_OUTPUT &&
      parser->current_token.type != TOKEN_REDIRECTION_INPUT &&
      parser->current_token.type != TOKEN_REDIRECT_AT)
    arguments(parser);
}

// Parse arguments
void arguments(Parser *parser) {
  argument(parser);
  if (parser->current_token.type == TOKEN_COMMAND_WORD ||
      parser->current_token.type == TOKEN_FILENAME)
    arguments(parser);
}

// Parse a single argument
void argument(Parser *parser) {
  parser_eat(parser, parser->current_token.type);
}

// Parse filename
void filename(Parser *parser) { parser_eat(parser, TOKEN_FILENAME); }

// Parse destination address
void dstaddr(Parser *parser) {
  parser_eat(parser, TOKEN_DSTADDR_IP);
  parser_eat(parser, TOKEN_DSTADDR_PORT);
}

// Parse IP address
void ip(Parser *parser) { parser_eat(parser, TOKEN_DSTADDR_IP); }

// Parse port
void port(Parser *parser) { parser_eat(parser, TOKEN_DSTADDR_PORT); }

// Main function
int main() {
  char input[] =
      "ls -l | grep '*.txt' > output.txt & echo hello >@ 192.168.1.10:8080";
  printf("Input: %s\n", input);
  Lexer *lexer = init_lexer(input);
  Parser *parser = init_parser(lexer);
  program(parser);

  printf("Parsing complete.\n");

  free(parser);
  free(lexer);
  return 0;
}
