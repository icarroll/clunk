#ifndef TTABLE_H
#define TTABLE_H

#include "thudlib.h"

struct tableentry
{
    hash_t hash;
    int depth;
    int score;
    int trmin;
    int dwmax;
};

void initttable(int memuse);
void ttput(struct tableentry entry);
struct tableentry * ttget(hash_t hash);

#endif // TTABLE_H
