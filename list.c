#include "list.h"

enum {STARTLIST = 100};

void initlist(struct movelist * list)
{
    list->used = 0;
    list->capacity = STARTLIST;
    list->moves = malloc(STARTLIST * sizeof(struct move));
}

static void grow(struct movelist * list)
{
    list->capacity *= 2;
    int newsize = list->capacity * sizeof(struct move);
    list->moves = realloc(list->moves, newsize);
}

struct move * next(struct movelist * list)
{
    int pos = list->used++;
    if (pos == list->capacity) grow(list);
    return & (list->moves[pos]);
}

void closelist(struct movelist * list)
{
    list->used = 0;
    list->capacity = 0;
    free(list->moves);
}
