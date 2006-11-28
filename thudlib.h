#ifndef THUDLIB_H
#define THUDLIB_H

#include "heap.h"
#include "list.h"
#include "board.h"
#include "move.h"

#include <stdbool.h>

struct genstate
{
    void * resume;
    int dx,dy;
    int dir;
    int dist;
    struct move move;
};

typedef struct move movegen_t(struct thudboard *, struct genstate *);

static char * BOOKFILENAME = "thud.book";

void setupgame(struct thudboard * board, int memuse);

struct move search(struct thudboard * board, int depth);
struct moveheap heapof(struct thudboard * board, struct movelist * list);
struct moveheap allmoves(struct thudboard * board, movegen_t nextmove);

int dwarfsearch(struct thudboard * board, int depth, int trmin, int dwmax);
struct movelist alldwarfmoves(struct thudboard * board);
struct move nextdwarfplay(struct thudboard * board, struct genstate * ctx);
void dodwarf(struct thudboard * board, struct move * move);
void undodwarf(struct thudboard * board, struct move * move);

void placedwarf(struct thudboard * board, struct coord to);
void placedwarfs(struct thudboard * board, int num, struct coord * tos);
void movedwarf(struct thudboard * board, struct coord from, struct coord to);
bool dwarfat(struct thudboard * board, struct coord pos);
void captdwarfs(struct thudboard * board, int num, struct coord * froms);

int trollsearch(struct thudboard * board, int depth, int trmin, int dwmax);
struct movelist alltrollmoves(struct thudboard * board);
int counttrollcapts(struct thudboard * board);
struct move nexttrollplay(struct thudboard * board, struct genstate * ctx);
void dotroll(struct thudboard * board, struct move * move);
void undotroll(struct thudboard * board, struct move * move);

void placetroll(struct thudboard * board, struct coord to);
void movetroll(struct thudboard * board, struct coord from, struct coord to);
bool trollat(struct thudboard * board, struct coord pos);
void capttroll(struct thudboard * board, struct coord from);

bool blockat(struct thudboard * board, struct coord pos);

void erase(struct thudboard * board);
void setup(struct thudboard * board);
char * pl(int n);
void show(struct thudboard * board);
int evaluate(struct thudboard * board);
bool legalmove(struct thudboard * board, struct move * move);
void domove(struct thudboard * board, struct move * move);
void undomove(struct thudboard * board, struct move * move);
void domoveforreal(struct thudboard * board, struct move * move);
void showpos(struct coord pos);
void showmove(struct move * move);

bool get(bitboard bits, struct coord pos);
void set(bitboard bits, struct coord pos);
void unset(bitboard bits, struct coord pos);
bool hasneighbor(bitboard bits, struct coord pos);
int countneighbors(bitboard bits, struct coord pos);
struct coord nextpos(bitboard bits, struct coord);
void erasebits(bitboard bits);

struct coord coord(int x, int y);
bool inbounds(struct coord);

void inithash(void);
void hashturn(struct thudboard * board);
void hashdwarf(struct thudboard * board, struct coord to);
void hashtroll(struct thudboard * board, struct coord to);

#endif // THUDLIB_H
