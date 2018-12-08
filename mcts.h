#pragma once

#include "mctree.h"

#define UCTK 1.0

struct move montecarlo(struct thudboard * board, int searchtime);
double mcts_step(struct thudboard * board, int movenum, int lastcapt);
double ucb1(double parent_visits, struct mctreenode * node);
double teamscale(struct mctreenode * node);
double mcts_simulate(struct mctreenode * node);
