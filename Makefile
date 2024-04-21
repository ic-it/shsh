# Created by: @ic-it
# Usage: make [all|clean|debug] [RELEASE=1]

VERSION=0.0.1
NAME=shsh

CC=clang
CINCLUDES=-I$(SRC_DIR)
CFLAGS=-Wno-gnu -Wall -Wextra -Werror -std=gnu11 -pedantic
ifeq ($(RELEASE), 1)
	CDEFINES=-DLOG_LEVEL=1 -DSHSH_VERSION=\"$(VERSION)\"
	CFLAGS=-O3 $(CDEFINES) $(CINCLUDES)
	SUBDIR=release
else
	CDEFINES=-DLOG_LEVEL=0 -DSHSH_VERSION=\"$(VERSION)-dev\"
	CFLAGS=-g3 -ggdb -O0 -fsanitize=address -fno-omit-frame-pointer $(CDEFINES) $(CINCLUDES) 
	SUBDIR=debug
endif

SRC_DIR=./src
OBJ_DIR=./obj/$(SUBDIR)
BIN_DIR=./bin/$(SUBDIR)

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
BIN=$(BIN_DIR)/$(NAME)

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
