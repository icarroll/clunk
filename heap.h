#ifndef HEAP_H
#define HEAP_H

#include "board.h"
#include "move.h"

struct scoredmove
{
    int score;
    struct move * move;
};

struct moveheap
{
    int used;
    int capacity;
    struct scoredmove * moves;
};

void initheap(struct moveheap * heap);
void closeheap(struct moveheap * heap);
void insert(struct moveheap * heap, int score, struct move * move);
struct move * pop(struct moveheap * heap);

#endif // HEAP_H
