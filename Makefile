CFLAGS = -g -std=gnu99
LDFLAGS = -lreadline

.PHONY: all
all: clunk

clunk: clunk.c thudlib.o
