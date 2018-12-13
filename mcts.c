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

    fprintf(stderr, "visited %d nodes with max depth %d\n", visited, farthest-movenum);

    // choose most visited child as move to make
    struct movelist moves = allmoves(& root);
    struct move bestmove;
    int bestvisits = INT_MIN;
    for (int ix=0 ; ix<moves.used ; ix+=1) {
        domove(& root, & moves.moves[ix]);

        struct tableentry * entry = ttget(root.hash);
        if (! entry) goto skip; //TODO what to do here?
        //printf("%d ",entry->visits);
        if (entry->visits > bestvisits) {
            bestmove = moves.moves[ix];
            bestvisits = entry->visits;
        }
skip:
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
        // create basic ttable entry
        double guess = heuristic(board);
        struct genstate ctx = newctx();
        ttput((struct tableentry) {board->hash, guess, 0, ctx, false});
        entry = ttget(board->hash);
    }

    entry->visits += 1;

    double guess;
    if (! entry->genfinished) {
        // some children remain unvisited, visit an unvisited child
        struct move move = nextplay(board, & entry->ctx);
        if (entry->ctx.resume == NULL) {
            entry->genfinished = true;
            goto actuallyfinished;
        }

        domove(board, & move);
        guess = mcts_simulate(board);
        undomove(board, & move);
    }
    else {
        struct movelist moves;   // keep the compiler happy about goto targets
actuallyfinished:
        // all children visited at least once, pick most promising via UCB1
        moves = allmoves(board);
        int bestix;
        double bestucb1 = -INFINITY;
        for (int ix=0 ; ix<moves.used ; ix+=1) {
            domove(board, & moves.moves[ix]);

            struct tableentry * childentry = ttget(board->hash);
            if (! childentry) {
                // create basic ttable entry
                double childguess = heuristic(board);
                struct genstate childctx = newctx();
                ttput((struct tableentry)
                        {board->hash, childguess, 1, childctx, false});
                childentry = ttget(board->hash);
            }

            double tryucb1 = ucb1(entry->scoreguess, entry->visits, board->isdwarfturn, childentry->visits);
            if (tryucb1 > bestucb1) {
                bestix = ix;
                bestucb1 = tryucb1;
            }

            undomove(board, & moves.moves[ix]);
        }

        struct move move = moves.moves[bestix];
        closelist(& moves);

        int newmovenum = movenum + 1;
        int newlastcapt = move.numcapts
                          ? newmovenum : lastcapt;
        domove(board, & move);
        guess = mcts_step(board, newmovenum, newlastcapt);
        undomove(board, & move);
    }

    entry->scoreguess = guess / entry->visits
                        + entry->scoreguess * (entry->visits-1) / entry->visits;

    return guess;
}

double ucb1(double scoreguess, int parent_visits, bool isdwarfturn,
            int child_visits) {
    double exploit = scoreguess * teamscale(isdwarfturn);
    double explore = sqrt(2 * log((double) parent_visits)
                         / (double) child_visits);
    return exploit + UCTK * explore;
}

double teamscale(bool isdwarfturn) {
    return (isdwarfturn ? 1.0 : -1.0) / (1000.0 * 32.0);
}

double mcts_simulate(struct thudboard * board) {
    // no rollout, just heuristic eval
    return (double) heuristic(board);
}
