CC := gcc

BINDIR := ./bin
BIN := $(BINDIR)/main

SRCDIR := ./src
SRC := $(wildcard $(SRCDIR)/*.c)

ifeq ($(OS),WINDOWS_NT)
	RFLAGS += -lgdi32 -lwinmm -I./raylib/src -L./raylib/src
endif
RFLAGS += -lraylib
RAYSRC := raylib_test.c
RAYBIN := $(RAYSRC:%.c=%.exe)

IDIR := ./include
LFLAGS := -I$(IDIR)
CFLAGS := -Wall -Wextra -pedantic $(LFLAGS) $(RFLAGS)
ifneq ($(OS),WINDOWS_NT)
	CFLAGS += -lm
endif

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
