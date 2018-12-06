#pragma once

#include "thudlib.h"

struct tableentry
{
    hash_t hash;
    double scoreguess;
    double visits;
};

void initttable(long memuse);
void ttput(struct tableentry entry);
struct tableentry * ttget(hash_t hash);
