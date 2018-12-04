#pragma once

#include "board.h"
#include "list.h"

struct mctreenode
{
    struct thudboard boardstate;
    struct movelist allmoves;
    int unexplored_moves;
    struct mctreenode * children;
    double scoreguess;
    double visits;
};

void initmctree(struct mctreenode * node, struct thudboard * board);
void closemctree(struct mctreenode * node);
