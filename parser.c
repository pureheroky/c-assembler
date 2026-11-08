#include "parser.h"
#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u8 final_binary[255];

typedef struct {
  Token name;
  int value;
} Constant;

Constant constants[10000];
int constant_count = 0;

int find_constant(Token name) {
  for (int i = 0; i < constant_count; i++) {
    if (constants[i].name.length == name.length &&
        strncmp(constants[i].name.source, name.source, name.length) == 0)
      return i;
  }
  return -1;
}

void collect_constants() {
  for (Token *t = tokens; t->type != TOKEN_EOF; t++) {
    if (t->type != TOKEN_CONST)
      continue;

    Token name = *(t + 1);
    if (name.type != TOKEN_IDENTIFIER || name.line != t->line) {
      printf("Error on line %d: Expected a constant name.\n", t->line);
      exit(1);
    }
    Token equal = *(t + 2);
    if (equal.type != TOKEN_EQUAL || equal.line != t->line) {
      printf("Error on line %d: Expected '=' after constant name.\n", t->line);
      exit(1);
    }
    Token value = *(t + 3);
    if (value.type != TOKEN_INTEGER || value.line != t->line) {
      printf("Error on line %d: Expected an integer constant value.\n",
             t->line);
      exit(1);
    }
    if ((t + 4)->type != TOKEN_EOF && (t + 4)->line == t->line) {
      printf("Error on line %d: Unexpected token after constant declaration.\n",
             t->line);
      exit(1);
    }
    if (find_constant(name) != -1) {
      printf("Error on line %d: Constant '%.*s' already declared.\n", name.line,
             name.length, name.source);
      exit(1);
    }
    constants[constant_count++] = (Constant){name, value.value};
    t += 3;
  }
}

struct Parser {
  Token *current;
  int next_byte;
} parser;

Token peek() { return *parser.current; }

Token consume() {
  Token t = *parser.current;
  if (t.type == TOKEN_EOF) {
    printf("Error on line %d: Unexpected end of file\n", t.line);
    exit(1);
  }
  parser.current++; // go to next token
  return t;
}

Token expect(TokenType type, char *error_message) {
  Token t = consume();
  if (t.type != type) {
    printf("Error on line %d: %s\n", t.line, error_message);
    exit(1);
  }
  return t;
}

void init_parser() {
  parser.current = tokens;
  parser.next_byte = 0;
}

void compile_byte(u8 x) {
  final_binary[parser.next_byte] = x;
  parser.next_byte += 1;
}

void parse_move() {
  Token regis = expect(TOKEN_REGISTER, "You can only move into a register");
  expect(TOKEN_COMMA, "Expected comma");
  Token rhs = consume();

  if (rhs.type == TOKEN_IDENTIFIER) {
    int index = find_constant(rhs);
    if (index == -1) {
      printf("Error on line %d: Unknown constant '%.*s'.\n", rhs.line,
             rhs.length, rhs.source);
      exit(1);
    }
    rhs.type = TOKEN_INTEGER;
    rhs.value = constants[index].value;
  }

  // compile loading constant into register 1
  if (rhs.type == TOKEN_INTEGER) {
    if (regis.value != 1) {
      printf("Error on line %d: Can only move constant into register 1.\n",
             regis.line);
      exit(1);
    }

    if (rhs.value > 63) {
      printf("Error on line %d: Can't move numbers bigger than 63 into "
             "register 1.\n",
             rhs.line);
      exit(1);
    }

    compile_byte(rhs.value);
    return;
  }

  // compile loading register into another register
  if (rhs.type == TOKEN_REGISTER) {
    int register_a = rhs.value << 3;
    int register_b = regis.value;
    int opcode = 0b10000000;
    compile_byte(opcode | register_a | register_b);
    return;
  }

  // invalid case
  printf("Error on line %d: Expected either a register or a constant for move "
         "operation.\n",
         rhs.line);
  exit(1);
}

void parse_line() {
  Token t = consume();

  if (t.type == TOKEN_MOV) {
    parse_move();
    return;
  }

  if (t.type == TOKEN_CONST) {
    expect(TOKEN_IDENTIFIER, "Expected constant name");
    expect(TOKEN_EQUAL, "Expected '='");
    expect(TOKEN_INTEGER, "Expected integer constant value");
    return;
  }

  if (t.type == TOKEN_IDENTIFIER) {
    printf("Compile error on line %d: unknown identifier.\n", t.line);
    exit(1);
  }

  printf("Error on line %d: Unexpected token.\n", t.line);
  exit(1);
}

void parse_lines() {
  init_parser();
  collect_constants();

  while (peek().type != TOKEN_EOF) {
    parse_line();
  }
}

void print_binary() {
  for (int i = 0; i < parser.next_byte; i++) {
    for (int j = 7; j >= 0; j--) {
      if (final_binary[i] & (1 << j)) {
        printf("1");
      } else {
        printf("0");
      }
    }
    printf("\n");
  }
}
