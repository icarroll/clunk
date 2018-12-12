#ifndef THUDLIB_H
#define THUDLIB_H

#include "heap.h"
#include "list.h"
#include "board.h"
#include "move.h"

#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <setjmp.h>

static char * BOOKFILENAME = "thud.book";

struct genstate
{
    void * resume;
    int dx,dy;
    int dir;
    int dist;
    struct move move;
};

enum {DRAW_DEADLINE = 10};

enum {FULL_WIDTH = INT_MAX};

enum {SEARCH_EXT = 2};

FILE * LOGF;
void setupgame(struct thudboard * board, long memuse);

struct move iterdeepen(struct thudboard * board, int searchtime);
struct move iterdeepenext(struct thudboard * board, int searchtime);
struct move zerowindow(struct thudboard * board, int depth,
                       time_t stoptime, jmp_buf stopsearch);
int mtdf(struct thudboard * board, int depth, int guess,
         time_t stoptime, jmp_buf stopsearch);
int _mtdf(struct thudboard * board, int depth,
          int guess, int min, int max,
          time_t stoptime, jmp_buf stopsearch);
int absearch(struct thudboard * board, int depth, int width,
             int trmin, int dwmax, struct move * bestmove,
             time_t stoptime, jmp_buf stopsearch);
int absearchext(struct thudboard * board, int depth, int width,
                int trmin, int dwmax, struct move * bestmove,
                time_t stoptime, jmp_buf stopsearch);

bool moveintothreat(struct thudboard * board, struct move * move);

struct moveheap heapof(struct thudboard * board, struct movelist * list);
struct movelist allmoves(struct thudboard * board);
void addmoves(int num, struct thudboard * board, struct genstate * ctx,
              struct movelist * list, struct moveheap * queue);

struct movelist alldwarfmoves(struct thudboard * board);
struct move nextdwarfplay(struct thudboard * board, struct genstate * ctx);
void dodwarf(struct thudboard * board, struct move * move);
void undodwarf(struct thudboard * board, struct move * move);

void placedwarf(struct thudboard * board, struct coord to);
void placedwarfs(struct thudboard * board, int num, struct coord * tos);
void movedwarf(struct thudboard * board, struct coord from, struct coord to);
bool dwarfat(struct thudboard * board, struct coord pos);
void captdwarfs(struct thudboard * board, int num, struct coord * froms);

struct movelist alltrollmoves(struct thudboard * board);
struct move nexttrollplay(struct thudboard * board, struct genstate * ctx);
int counttrollcapts(struct thudboard * board);
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
int heuristic(struct thudboard * board);
int score_game(struct thudboard * board);
bool legalmove(struct thudboard * board, struct move * move);
bool legaldwarfmove(struct thudboard * board, struct move * move);
bool legaltrollmove(struct thudboard * board, struct move * move);
bool gameover(struct thudboard * board);
void domove(struct thudboard * board, struct move * move);
void undomove(struct thudboard * board, struct move * move);
void domoveupdatecapts(struct thudboard * board, struct move * move);
void skipspace(char ** input);
bool getpos(char ** input, struct coord * pos);
void fshowpos(FILE * stream, struct coord pos);
void showpos(struct coord pos);
void fshowmove(FILE * stream, struct move * move);
void showmove(struct move * move);

enum {HIGHBIT = 1 << (SIZE - 1)};
bool get(bitboard bits, struct coord pos);
void set(bitboard bits, struct coord pos);
void unset(bitboard bits, struct coord pos);
bool hasneighbor(bitboard bits, struct coord pos);
int countneighbors(bitboard bits, struct coord pos);
struct coord nextpos(bitboard bits, struct coord);
void erasebits(bitboard bits);

bool inbounds(struct coord);

void inithash(void);
void hashturn(struct thudboard * board);
void hashdwarf(struct thudboard * board, struct coord to);
void hashtroll(struct thudboard * board, struct coord to);

#endif // THUDLIB_H
