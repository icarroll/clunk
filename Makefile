CFLAGS = -O3 -std=gnu99

TARGETS = clunk book playthud gendata
OBJS = thudlib.o heap.o list.o linenoise.o ttable.o

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) *.o

clunk: clunk.c $(OBJS)

book: book.c $(filter-out ttable.o,$(OBJS))

playthud: playthud.c $(OBJS)

gendata: gendata.c $(OBJS)
