CFLAGS = -g -std=gnu99
LDFLAGS = -lreadline

TARGETS = clunk book

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm $(TARGETS) *.o

clunk: clunk.c thudlib.o ttable.o

book: book.c thudlib.o
