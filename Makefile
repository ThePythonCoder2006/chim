CC := gcc

BINDIR := ./bin
BIN := $(BINDIR)/main

SRCDIR := ./src
SRC := $(wildcard $(SRCDIR)/*.c)

RFLAGS := -I./raylib/src -L./raylib/src -lraylib -lgdi32 -lwinmm
RAYSRC := raylib_test.c
RAYBIN := $(RAYSRC:%.c=%.exe)

IDIR := ./include
LFLAGS := -I$(IDIR)
CFLAGS := -Wall -Wextra -pedantic -ggdb $(LFLAGS) $(RFLAGS)

default: run

$(BIN): $(SRC) | $(BINDIR)
	$(CC) $^ -o $@ $(CFLAGS)

run: $(BIN)
	$^

db: $(BIN)
	gdb $^

ray $(RAYBIN): $(RAYSRC)
	$(CC) $< -o $(RAYBIN) $(CFLAGS) $(RFLAGS)
	./$(RAYBIN)

$(BINDIR):
	-mkdir $@