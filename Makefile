CFLAGS = -O3 -std=gnu99
# CFLAGS = -g -std=gnu99
LDLIBS = -lreadline -lm

TARGETS = clunk playthud
OBJS = thudlib.o heap.o list.o mcts.o mctree.o ttable.o

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) *.o

clunk: clunk.c $(OBJS)

playthud: playthud.c $(OBJS)
