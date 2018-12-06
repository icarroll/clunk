#include <math.h>
#include <stdlib.h>

#include "thudlib.h"
#include "mctree.h"
#include "mcts.h"

// TODO move this into game state struct
int movenum = 0;   // move number of game
int lastcapt = 0;   // move number of last capture

int farthest;   // highest movenum visited this search
int visited;   // total nodes visited

struct move montecarlo(struct thudboard * board, int searchtime) {
    time_t stoptime = time(NULL) + searchtime;

    struct mctreenode * root = malloc(sizeof(struct mctreenode));
    initmctree(root, board);

    farthest = movenum;
    visited = 1;

    jmp_buf stopsearch;
    if (setjmp(stopsearch) == 0) while (time(NULL) < stoptime)
    {
        mcts_step(root, movenum, lastcapt);
    }

    printf("visited %d nodes with max depth %d\n", visited, farthest-movenum);

    // choose most visited child as move to make
    struct move bestmove = root->allmoves.moves[0];
    int bestvisited = root->children[0].visits;
    for (int ix=1 ; ix<root->allmoves.used ; ix+=1) {
        if (root->children[ix].visits > bestvisited) {
            bestmove = root->allmoves.moves[ix];
            bestvisited = root->children[ix].visits;
        }
    }
    printf("chose "); showmove(& bestmove);

    printf("freeing tree\n"); fflush(stdout);
    closemctree(root); //TODO reuse relevant subtree
    free(root);
    printf("freed tree\n"); fflush(stdout);

    return bestmove;
}

double mcts_step(struct mctreenode * node, int movenum, int lastcapt) {
    if (movenum > farthest) farthest = movenum;

    node->visits += 1;
    visited += 1;   // count all nodes visited

    if (movenum - lastcapt > DRAW_DEADLINE) {
        // terminal node, game over
        return (double) score_game(& node->boardstate);
    }

    double guess;
    if (node->unexplored_moves) {
        // some children remain unvisited, visit an unvisited child
        int ix = node->allmoves.used - node->unexplored_moves;
        node->unexplored_moves -= 1;

        struct mctreenode * child = & node->children[ix];

        struct thudboard board = node->boardstate;
        struct move move = node->allmoves.moves[ix];
        domove(& board, & move);

        initmctree(child, & board);

        guess = mcts_simulate(child);
        child->scoreguess = guess;
    }
    else {
        // all children visited at least once, pick most promising via UCB1
        int choice = 0;
        double value = ucb1(node->visits, & node->children[choice]);
        for (int ix=0 ; ix<node->allmoves.used ; ix+=1) {
            double tryvalue = ucb1(node->visits, & node->children[ix]);
            if (tryvalue > value) {
                choice = ix;
                value = tryvalue;
            }
        }

        int newmovenum = movenum + 1;
        int newlastcapt = node->allmoves.moves[choice].numcapts
                          ? newmovenum : lastcapt;
        guess = mcts_step(& node->children[choice], newmovenum, newlastcapt);
    }

    node->scoreguess = guess / node->visits
                       + node->scoreguess * (node->visits-1) / node->visits;

    return guess;
}

double ucb1(double parent_visits, struct mctreenode * node) {
    return node->scoreguess * teamscale(node)
           + UCTK * sqrt(2 * log(parent_visits) / node->visits);
}

double teamscale(struct mctreenode * node) {
    return (node->boardstate.isdwarfturn ? 1.0 : -1.0) / (1000.0 * 32.0);
}

double mcts_simulate(struct mctreenode * node) {
    // no rollout, just heuristic eval
    return (double) heuristic(& node->boardstate);
}

