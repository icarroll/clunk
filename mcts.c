#include <math.h>
#include <stdlib.h>

#include "mctree.h"
#include "mcts.h"
#include "thudlib.h"
#include "ttable.h"

// TODO move this into game state struct
int movenum = 0;   // move number of game
int lastcapt = 0;   // move number of last capture

int farthest;   // highest movenum visited this search
int visited;   // total nodes visited

void die(char * msg) {
    fputs(msg, stderr);
    putc('\n', stderr);
    exit(1);
}

struct move montecarlo(struct thudboard * board, int searchtime) {
    time_t stoptime = time(NULL) + searchtime;

    farthest = movenum;
    visited = 1;

    struct thudboard root = * board;

    jmp_buf stopsearch; // not really necessary since a single step is short
    if (setjmp(stopsearch) == 0) while (time(NULL) < stoptime)
    {
        mcts_step(& root, movenum, lastcapt);
    }

    printf("visited %d nodes with max depth %d\n", visited, farthest-movenum);

    // choose most visited child as move to make
    struct movelist moves = allmoves(& root);
    struct move bestmove;
    int bestvisits = INT_MIN;
    for (int ix=0 ; ix<moves.used ; ix+=1) {
        domove(& root, & moves.moves[ix]);

        struct tableentry * entry = ttget(root.hash);
        if (! entry) continue;
        if (entry->visits > bestvisits) {
            bestmove = moves.moves[ix];
            bestvisits = entry->visits;
        }
        undomove(& root, & moves.moves[ix]);
    }

    if (bestvisits == INT_MIN) die("no move found");

    return bestmove;
}

double mcts_step(struct thudboard * board, int movenum, int lastcapt) {
    if (movenum > farthest) farthest = movenum;

    visited += 1;   // count all nodes visited

    if (movenum - lastcapt > DRAW_DEADLINE) {
        // terminal node, game over
        return (double) score_game(board);
    }

    struct tableentry * entry = ttget(board->hash);
    if (! entry) {
        // first visit, put table entry
        struct genstate ctx; ctx.resume = NULL;
        ttput((struct tableentry) {board->hash, 0, 0, ctx});
        entry = ttget(board->hash);
    }

    entry->visits += 1;

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

