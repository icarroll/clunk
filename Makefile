CFLAGS = -O3 -std=gnu99

TARGETS = clunk book playthud
OBJS = thudlib.o heap.o list.o linenoise.o

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm $(TARGETS) *.o

clunk: clunk.c $(OBJS) ttable.o

book: book.c $(OBJS)

playthud: playthud.c $(OBJS) ttable.o
