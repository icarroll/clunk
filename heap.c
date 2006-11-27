#include "heap.h"
#include "thudlib.h"

#include <stdlib.h>

enum {STARTHEAP = 100};

void initheap(struct moveheap * heap)
{
    heap->used = 0;
    heap->capacity = STARTHEAP;
    heap->moves = malloc(STARTHEAP * sizeof(struct scoredmove));
}

static void grow(struct moveheap * heap)
{
    heap->capacity *= 2;
    int newsize = heap->capacity * sizeof(struct scoredmove);
    heap->moves = realloc(heap->moves, newsize);
}

void closeheap(struct moveheap * heap)
{
    heap->used = 0;
    heap->capacity = 0;
    free(heap->moves);
}

static int parent(int i)
{
    return (i + 1) / 2 - 1;
}

static int left(int i)
{
    return i * 2 + 1;
}

static int right(int i)
{
    return i * 2 + 2;
}

static int score(struct moveheap * heap, int i)
{
    return heap->moves[i].score;
}

static void swap(struct moveheap * heap, int i, int j)
{
    struct scoredmove temp = heap->moves[i];
    heap->moves[i] = heap->moves[j];
    heap->moves[j] = temp;
}

static void heapup(struct moveheap * heap, int i)
{
    if (i > 0)
    {
        int p = parent(i);
        if (score(heap, i) > score(heap, p))
        {
            swap(heap, i, p);
            heapup(heap, p);
        }
    }
}

static void heapdown(struct moveheap * heap, int i)
{
    int l = left(i);
    int r = right(i);

    if (r < heap->used)
    {
        int j = i;
        if (score(heap, l) > score(heap, i)) j = l;
        if (score(heap, r) > score(heap, j)) j = r;

        if (i != j)
        {
            swap(heap, i, j);
            heapdown(heap, j);
        }
    }
    else if (l < heap->used
             && score(heap, i) < score(heap, l))
    {
        swap(heap, i, l);
    }
}

void insert(struct moveheap * heap, int score, struct move * move)
{
    if (heap->used == heap->capacity) grow(heap);

    heap->moves[heap->used++] = (struct scoredmove) {score, move};
    heapup(heap, heap->used - 1);
}

struct move * pop(struct moveheap * heap)
{
    struct move * move = heap->moves[0].move;

    heap->moves[0] = heap->moves[--heap->used];
    heapdown(heap, 0);

    return move;
}
