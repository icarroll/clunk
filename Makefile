CFLAGS = -g -std=gnu99 -pg
LDFLAGS = -lreadline -pg

TARGETS = clunk book playthud
OBJS = thudlib.o heap.o list.o

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm $(TARGETS) *.o

clunk: clunk.c $(OBJS) ttable.o

book: book.c $(OBJS)

playthud: playthud.c $(OBJS) ttable.o
