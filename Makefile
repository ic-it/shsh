
SRC_DIR=./src
OBJ_DIR=./obj
BIN_DIR=./bin

CC=clang
CFLAGS=-Wall -Wextra -Werror -g -I$(SRC_DIR)

SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
BIN=$(BIN_DIR)/main

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)

.PHONY: all clean
