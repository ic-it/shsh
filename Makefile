SRC_DIR=./src
OBJ_DIR=./obj
BIN_DIR=./bin

CC=clang
CDEFINES=-DLOG_LEVEL=0
CINCLUDES=-I$(SRC_DIR)
CFLAGS=-Wno-gnu -Wall -Wextra -Werror -g3 -ggdb -O0 -std=c11 -pedantic -fsanitize=address -fno-omit-frame-pointer $(CDEFINES) $(CINCLUDES) 

DBGR=gdb
DBGR_ARGS=\
	--eval-command="set disassembly-flavor intel" \
	--eval-command="layout asm" \
	--eval-command="layout reg" \
	--eval-command="ref" \
	--eval-command="break _start" \
	--eval-command="run" \
	--eval-command="si"

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

debug: $(BIN)
	$(DBGR) $(DBGR_ARGS) $(BIN)

.PHONY: all clean
