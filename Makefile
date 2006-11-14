CFLAGS = -g -std=gnu99 -lreadline

.PHONY: all
all: clunk

clunk: clunk.c thudlib.c
