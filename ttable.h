#pragma once

#include "thudlib.h"

struct tableentry
{
    hash_t hash;
    double scoreguess;
    int visits;
    struct genstate ctx;
    bool genfinished;
};

void initttable(long memuse);
void ttput(struct tableentry entry);
struct tableentry * ttget(hash_t hash);
