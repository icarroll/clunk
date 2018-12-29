CFLAGS = -O3 -std=gnu99 #-DHAVE_SSE2 -DSFMT_MEXP=19937 
# CFLAGS = -g -std=gnu99
LDLIBS = -lreadline -lm

TARGETS = clunk playthud
OBJS = thudlib.o heap.o list.o mcts.o mctree.o ttable.o SFMT.o

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) *.o

clunk: clunk.c $(OBJS)

playthud: playthud.c $(OBJS)

SFMT.o: SFMT.c SFMT.h SFMT-common.h SFMT-params.h SFMT-params19937.h SFMT-sse2.h SFMT-sse2-msc.h
	gcc -O3 -finline-functions -fomit-frame-pointer -DNDEBUG -DHAVE_SSE2 -DSFMT_MEXP=19937 -fno-strict-aliasing --param max-inline-insns-single=1800 -Wmissing-prototypes -Wall -std=gnu99 -msse2 -c -o SFMT.o SFMT.c
