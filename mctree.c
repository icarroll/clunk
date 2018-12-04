#include "mctree.h"
#include "thudlib.h"

void initmctree(struct mctreenode * node, struct thudboard * board) {
    node->boardstate = * board;
    node->allmoves = allmoves(board);
    int movecount = node->allmoves.used;
    node->unexplored_moves = movecount;
    node->children = malloc(movecount * sizeof(struct mctreenode));
    node->scoreguess = 0;
    node->visits = 1;
}

void closemctree(struct mctreenode * node) {
    for (int ix=0 ; ix<node->allmoves.used ; ix+=1) {
        closemctree(& node->children[ix]);
    }
    free(node->children);

    closelist(& (node->allmoves));
}
