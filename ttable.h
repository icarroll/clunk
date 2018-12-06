#pragma once

#include "thudlib.h"

struct tableentry
{
    hash_t hash;
    int depth;
    int worth;
    int scoreguess;
    int min;
    int max;
};

void initttable(long memuse);
void ttput(struct tableentry entry);
struct tableentry * ttget(hash_t hash);
