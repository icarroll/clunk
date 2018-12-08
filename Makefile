# CFLAGS = -O3 -std=gnu99
CFLAGS = -g -std=gnu99
LDLIBS = -lreadline -lm

TARGETS = clunk
OBJS = thudlib.o heap.o list.o mcts.o mctree.o

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) *.o

clunk: clunk.c $(OBJS) ttable.o
