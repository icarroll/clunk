#pragma once

#include "mctree.h"

#define UCTK 10e-19

struct move montecarlo(struct thudboard * board, int searchtime);
double mcts_step(struct thudboard * board, int movenum, int lastcapt);
double ucb1(double scoreguess, int parent_visits, bool isdwarfturn,
            int child_visits);
double teamscale(bool isdwarfturn);
double mcts_simulate(struct thudboard * board);
