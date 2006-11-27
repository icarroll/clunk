#ifndef TTABLE_H
#define TTABLE_H

#include "thudlib.h"

struct tableentry
{
    hash_t hash;
    int depth;
    int trmin;
    int dwmax;
};

void initttable(int memuse);
void ttput(struct thudboard * board, int depth, int trmin, int dwmax);
struct tableentry * ttget(hash_t hash);

#endif // TTABLE_H
