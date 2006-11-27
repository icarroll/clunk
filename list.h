#ifndef LIST_H
#define LIST_H

#include "board.h"
#include "move.h"

#include <stdlib.h>

struct movelist
{
    int used;
    int capacity;
    struct move * moves;
};

void initlist(struct movelist * list);
struct move * next(struct movelist * list);
void closelist(struct movelist * list);

#endif // LIST_H
