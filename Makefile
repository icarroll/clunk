CFLAGS = -g -std=gnu99
LDFLAGS = -lreadline

TARGETS = clunk book
OBJS = thudlib.o heap.o list.o

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm $(TARGETS) *.o

clunk: clunk.c $(OBJS) ttable.o

book: book.c $(OBJS)
