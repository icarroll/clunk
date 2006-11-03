#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include "coroutine.h"

const int XSIZE = 15;
const int YSIZE = 15;

typedef uint16_t bitboard[15];

struct thudboard
{
    bitboard dwarfs;
    bitboard trolls;
    bitboard blocks;
    bool isdwarfturn;
};

struct pos
{
    int x;
    int y;
};

struct move
{
    bool isdwarfmove;
    struct pos from;
    struct pos to;
    int numcapts;
    struct pos capts[8];
};

char * stdlayout[] =
{
    "#####dd.dd#####",
    "####d.....d####",
    "###d.......d###",
    "##d.........d##",
    "#d...........d#",
    "d.............d",
    "d.....TTT.....d",
    "......T#T......",
    "d.....TTT.....d",
    "d.............d",
    "#d...........d#",
    "##d.........d##",
    "###d.......d###",
    "####d.....d####",
    "#####dd.dd#####",
    NULL
};

struct move search(struct thudboard * board, int depth);

int dwarfsearch(struct thudboard * board, int depth, int alpha, int beta);
struct move nextdwarfplay(struct thudboard * board, ccrContParam);
void dodwarf(struct thudboard * board, struct move * play);
void undodwarf(struct thudboard * board, struct move * play);

int trollsearch(struct thudboard * board, int depth, int alpha, int beta);
struct move nexttrollplay(struct thudboard * board, ccrContParam);
void dotroll(struct thudboard * board, struct move * play);
void undotroll(struct thudboard * board, struct move * play);

void erase(struct thudboard * board);
void setup(struct thudboard * board);
void show(struct thudboard * board);
int evaluate(struct thudboard * board);

bool get(bitboard bits, struct pos xy);
void set(bitboard bits, struct pos xy);
void unset(bitboard bits, struct pos xy);
int census(bitboard bits);

int main(int numargs, char * args[])
{
    struct thudboard board_data;
    struct thudboard * board = & board_data;

    setup(board);
    show(board);
}

struct move search(struct thudboard * board, int depth)
{
    int alpha = INT_MIN;
    int beta = INT_MAX;

    struct move bestmove;
    struct move play;

    ccrContext ctx = NULL;

    if (board->isdwarfturn)
    {
        while (true)
        {
            play = nextdwarfplay(board, ctx);
            if (! ctx) break;

            dodwarf(board, &play);

            int result = trollsearch(board, depth-1, alpha, beta);
            if (result < beta)
            {
                beta = result;
                bestmove = play;
            }

            undodwarf(board, & play);
        }
    }
    else
    {
        while (true)
        {
            play = nexttrollplay(board, ctx);
            if (! ctx) break;

            dotroll(board, & play);

            int result = dwarfsearch(board, depth-1, alpha, beta);
            if (result > alpha)
            {
                alpha = result;
                bestmove = play;
            }

            undotroll(board, & play);
        }
    }

    return bestmove;
}

int dwarfsearch(struct thudboard * board, int depth, int alpha, int beta)
{
    if (depth < 1) return evaluate(board);

    ccrContext ctx = NULL;

    while (true)
    {
        struct move play = nextdwarfplay(board, ctx);
        if (! ctx) break;

        dodwarf(board, & play);

        int result = trollsearch(board, depth-1, alpha, beta);
        if (result < beta) beta = result;

        undodwarf(board, & play);

        if (alpha >= beta) return alpha;
    }

    return beta;
}

struct move nextdwarfplay(struct thudboard * board, ccrContParam)
{
}

void dodwarf(struct thudboard * board, struct move * play)
{
    unset(board->dwarfs, play->from);
    if (play->numcapts != 0) unset(board->trolls, play->capts[0]);
    set(board->dwarfs, play->to);
}

void undodwarf(struct thudboard * board, struct move * play)
{
    unset(board->dwarfs, play->to);
    if (play->numcapts != 0) set(board->trolls, play->capts[0]);
    set(board->dwarfs, play->from);
}

int trollsearch(struct thudboard * board, int depth, int alpha, int beta)
{
    if (depth < 1) return evaluate(board);

    ccrContext ctx = NULL;

    while (true)
    {
        struct move play = nexttrollplay(board, ctx);
        if (! ctx) break;

        dotroll(board, & play);

        int result = dwarfsearch(board, depth-1, alpha, beta);
        if (result > alpha) alpha = result;

        undotroll(board, & play);

        if (alpha >= beta) return beta;
    }

    return alpha;
}

struct move nexttrollplay(struct thudboard * board, ccrContParam)
{
}

void dotroll(struct thudboard * board, struct move * play)
{
    unset(board->trolls, play->from);
    for (int i=0; i < play->numcapts; ++i)
    {
        unset(board->dwarfs, play->capts[i]);
    }
    set(board->trolls, play->to);
}

void undotroll(struct thudboard * board, struct move * play)
{
    unset(board->trolls, play->to);
    for (int i=0; i < play->numcapts; ++i)
    {
        set(board->dwarfs, play->capts[i]);
    }
    set(board->trolls, play->from);
}

void erase(struct thudboard * board)
{
    memset(board, 0, sizeof(struct thudboard));
}

struct pos pos(int x, int y)
{
    struct pos p = {x,y};
    return p;
}

void die(char * msg)
{
    fprintf(stderr, msg);
    exit(1);
}

void setup(struct thudboard * board)
{
    erase(board);

    board->isdwarfturn = true;

    for (int x=0; x < XSIZE; ++x)
    {
        for (int y=0; y < YSIZE; ++y)
        {
            switch (stdlayout[y][x])
            {
            case 'd':
                set(board->dwarfs, pos(x,y));
                break;
            case 'T':
                set(board->trolls, pos(x,y));
                break;
            case '#':
                set(board->blocks, pos(x,y));
                break;
            case '.':
                break;
            default:
                die("unknown board character\n");
                break;
            }
        }
    }
}

void show(struct thudboard * board)
{
    for (int x=0; x < XSIZE; ++x)
    {
        for (int y=0; y < YSIZE; ++y)
        {
            putchar(' ');

            if (get(board->dwarfs, pos(x,y))) putchar('d');
            else if (get(board->trolls, pos(x,y))) putchar('T');
            else if (get(board->blocks, pos(x,y))) putchar('#');
            else putchar('.');
        }
        putchar('\n');
    }
    fflush(stdout);
}

int evaluate(struct thudboard * board)
{
    int dwarfs = 1000 * census(board->dwarfs);
    int trolls = 4000 * census(board->trolls);

    int group = 0;

    return trolls - (dwarfs + group);
}

uint16_t bit(int x)
{
    return (uint16_t) 0x8000 >> x;
}

bool get(bitboard bits, struct pos pos)
{
    return (bits[pos.y] & bit(pos.x)) != 0;
}

void set(bitboard bits, struct pos pos)
{
    bits[pos.y] |= bit(pos.x);
}

void unset(bitboard bits, struct pos pos)
{
    bits[pos.y] &= ~bit(pos.x);
}

int census(bitboard bits)
{
    int total = 0;

    for (int row=0; row < YSIZE; ++row)
    {
        /* from Software Optimization Guide for AMD Athlon (tm) 64
         *      and Opteron (tm) Processors */
        unsigned int w = bits[row] - ((bits[row] >> 1) & 0x55555555);
        unsigned int x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
        unsigned int c = ((x + (x >> 4) & 0x0f0f0f0f) * 0x01010101) >> 24;
        total += c;
    }

    return total;
}
