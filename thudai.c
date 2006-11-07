#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include "coroutine.h"

const int SIZE = 15;

typedef uint16_t bitboard[15];

struct thudboard
{
    bool isdwarfturn;

    int numdwarfs;
    int numtrolls;

    int dwarfclump;

    bitboard dwarfs;
    bitboard trolls;
    bitboard blocks;
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

void placedwarf(struct thudboard * board, struct pos to);
void placedwarfs(struct thudboard * board, int num, struct pos * tos);
void movedwarf(struct thudboard * board, struct pos from, struct pos to);
bool dwarfat(struct thudboard * board, struct pos pos);
void captdwarfs(struct thudboard * board, int num, struct pos * froms);

int trollsearch(struct thudboard * board, int depth, int alpha, int beta);
struct move nexttrollplay(struct thudboard * board, ccrContParam);
void dotroll(struct thudboard * board, struct move * play);
void undotroll(struct thudboard * board, struct move * play);

void placetroll(struct thudboard * board, struct pos to);
void movetroll(struct thudboard * board, struct pos from, struct pos to);
bool trollat(struct thudboard * board, struct pos pos);
void capttroll(struct thudboard * board, struct pos from);

bool blockat(struct thudboard * board, struct pos pos);

void erase(struct thudboard * board);
void setup(struct thudboard * board);
void show(struct thudboard * board);
int evaluate(struct thudboard * board);

bool get(bitboard bits, struct pos pos);
void set(bitboard bits, struct pos pos);
void unset(bitboard bits, struct pos pos);
int census(bitboard bits);
bool hasneighbor(bitboard bits, struct pos pos);
int countneighbors(bitboard bits, struct pos pos);
struct pos nextpos(bitboard bits, ccrContParam);

struct pos pos(int x, int y);
bool inbounds(struct pos);

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

const int NUMDIRS = 8;
const int DXS[] = {0, 1, 1, 1, 0, -1, -1, -1};
const int DYS[] = {-1, -1, 0, 1, 1, 1, 0, -1};

struct move nextdwarfplay(struct thudboard * board, ccrContParam)
{
    ccrBeginContext;
    struct move move;
    ccrContext subctx;
    int dir;
    int dist;
    int dx,dy;
    ccrEndContext(ctx);

    ccrBegin(ctx);
    ctx->move.isdwarfmove = true;

    // generate attacks
    ctx->move.numcapts = 1;
    ctx->subctx = NULL;
    while (true)
    {
        ctx->move.from = nextpos(board->dwarfs, ctx->subctx);
        if (! ctx->subctx) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DXS[ctx->dir];
            ctx->dy = DYS[ctx->dir];

            for (ctx->dist = 1; ctx->dist < SIZE ; ++ctx->dist)
            {
                ctx->move.to.x = ctx->move.from.x + ctx->dx * ctx->dist;
                ctx->move.to.y = ctx->move.from.y + ctx->dy * ctx->dist;
                if (! inbounds(ctx->move.to)) break;

                if (dwarfat(board, ctx->move.to)
                    || blockat(board, ctx->move.to)) break;

                struct pos check;
                check.x = ctx->move.from.x - ctx->dx * (ctx->dist-1);
                check.y = ctx->move.from.y - ctx->dy * (ctx->dist-1);
                if (! inbounds(check) || ! dwarfat(board, check)) break;

                if (trollat(board, ctx->move.to))
                {
                    ctx->move.capts[0] = ctx->move.to;
                    ccrReturn(ctx->move);
                    break;
                }
            }
        }
    }

    // generate moves
    ctx->move.numcapts = 0;
    // ctx->subctx is already NULL
    while (true)
    {
        ctx->move.from = nextpos(board->dwarfs, ctx->subctx);
        if (! ctx->subctx) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DXS[ctx->dir];
            ctx->dy = DYS[ctx->dir];

            for (ctx->dist = 1; ctx->dist < SIZE ; ++ctx->dist)
            {
                ctx->move.to.x = ctx->move.from.x + ctx->dx * ctx->dist;
                ctx->move.to.y = ctx->move.from.y + ctx->dy * ctx->dist;
                if (! inbounds(ctx->move.to)) break;

                if (dwarfat(board, ctx->move.to)
                    || trollat(board, ctx->move.to)
                    || blockat(board, ctx->move.to)) break;

                ccrReturn(ctx->move);
            }
        }
    }
    ccrFinishV; //???
}

void dodwarf(struct thudboard * board, struct move * play)
{
    if (play->numcapts > 0) capttroll(board, play->capts[0]);
    movedwarf(board, play->from, play->to);
}

void undodwarf(struct thudboard * board, struct move * play)
{
    movedwarf(board, play->to, play->from);
    if (play->numcapts > 0) placetroll(board, play->capts[0]);
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
    ccrBeginContext;
    struct move move;
    ccrContext subctx;
    int dir;
    int dist;
    int dx,dy;
    ccrEndContext(ctx);

    ccrBegin(ctx);
    ctx->move.isdwarfmove = false;

    // generate attacks
    ctx->subctx = NULL;
    while (true)
    {
        ctx->move.from = nextpos(board->trolls, ctx->subctx);
        if (! ctx->subctx) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DXS[ctx->dir];
            ctx->dy = DYS[ctx->dir];

            for (ctx->dist = 1; ctx->dist < SIZE ; ++ctx->dist)
            {
                ctx->move.to.x = ctx->move.from.x + ctx->dx * ctx->dist;
                ctx->move.to.y = ctx->move.from.y + ctx->dy * ctx->dist;
                if (! inbounds(ctx->move.to)) break;

                if (dwarfat(board, ctx->move.to)
                    || trollat(board, ctx->move.to)
                    || blockat(board, ctx->move.to)) break;

                struct pos check;
                check.x = ctx->move.from.x - ctx->dx * (ctx->dist-1);
                check.y = ctx->move.from.y - ctx->dy * (ctx->dist-1);
                if (! inbounds(check) || ! trollat(board, check)) break;

                if (hasneighbor(board->dwarfs, ctx->move.to))
                {
                    for (int i=0; i < NUMDIRS; ++i)
                    {
                        struct pos capt = {ctx->move.to.x + DXS[i],
                                           ctx->move.to.y + DYS[i]};
                        if (dwarfat(board, capt))
                        {
                            ctx->move.capts[ctx->move.numcapts++] = capt;
                        }
                    }
                    ccrReturn(ctx->move);
                }
            }
        }
    }

    // generate moves
    ctx->move.numcapts = 0;
    // ctx->subctx is already NULL
    while (true)
    {
        ctx->move.from = nextpos(board->dwarfs, ctx->subctx);
        if (! ctx->subctx) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DXS[ctx->dir];
            ctx->dy = DYS[ctx->dir];

            for (ctx->dist = 1; ctx->dist < SIZE ; ++ctx->dist)
            {
                ctx->move.to.x = ctx->move.from.x + ctx->dx * ctx->dist;
                ctx->move.to.y = ctx->move.from.y + ctx->dy * ctx->dist;
                if (! inbounds(ctx->move.to)) break;

                if (dwarfat(board, ctx->move.to)
                    || trollat(board, ctx->move.to)
                    || blockat(board, ctx->move.to)) break;

                ccrReturn(ctx->move);
            }
        }
    }
    ccrFinishV; //???
}

void dotroll(struct thudboard * board, struct move * play)
{
    captdwarfs(board, play->numcapts, play->capts);
    movetroll(board, play->from, play->to);
}

void undotroll(struct thudboard * board, struct move * play)
{
    movetroll(board, play->to, play->from);
    placedwarfs(board, play->numcapts, play->capts);
}

void erase(struct thudboard * board)
{
    memset(board, 0, sizeof(struct thudboard));
}

void setup(struct thudboard * board)
{
    erase(board);

    board->isdwarfturn = true;

    board->numdwarfs = 0;
    board->numtrolls = 0;

    for (int x=0; x < SIZE; ++x)
    {
        for (int y=0; y < SIZE; ++y)
        {
            switch (stdlayout[y][x])
            {
            case 'd':
                placedwarf(board, pos(x,y));
                break;
            case 'T':
                placetroll(board, pos(x,y));
                break;
            case '#':
                set(board->blocks, pos(x,y));
                break;
            case '.':
                break;
            default:
                fprintf(stderr, "unknown board character\n");
                exit(1);
            }
        }
    }
}

void show(struct thudboard * board)
{
    for (int x=0; x < SIZE; ++x)
    {
        for (int y=0; y < SIZE; ++y)
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
    return 4000 * board->numtrolls
           - 1000 * board->numdwarfs
           - board->dwarfclump;
}

void placedwarf(struct thudboard * board, struct pos to)
{
    board->dwarfclump += countneighbors(board->dwarfs, to);
    set(board->dwarfs, to);
    board->numdwarfs += 1;
}

void placedwarfs(struct thudboard * board, int num, struct pos * tos)
{
    for (int i=0; i < num; ++i)
    {
        board->dwarfclump += countneighbors(board->dwarfs, tos[i]);
        set(board->dwarfs, tos[i]);
    }
    board->numdwarfs += num;
}

void movedwarf(struct thudboard * board, struct pos from, struct pos to)
{
    unset(board->dwarfs, from);
    board->dwarfclump -= countneighbors(board->dwarfs, from);
    board->dwarfclump += countneighbors(board->dwarfs, to);
    set(board->dwarfs, to);
}

bool dwarfat(struct thudboard * board, struct pos pos)
{
    return get(board->dwarfs, pos);
}

void captdwarfs(struct thudboard * board, int num, struct pos * froms)
{
    for (int i=0; i < num; ++i)
    {
        unset(board->dwarfs, froms[i]);
        board->dwarfclump -= countneighbors(board->dwarfs, froms[i]);
    }
    board->numdwarfs -= num;
}

void placetroll(struct thudboard * board, struct pos to)
{
    set(board->trolls, to);
    board->numtrolls += 1;
}

void movetroll(struct thudboard * board, struct pos from, struct pos to)
{
    unset(board->trolls, from);
    set(board->trolls, to);
}

bool trollat(struct thudboard * board, struct pos pos)
{
    return get(board->trolls, pos);
}

void capttroll(struct thudboard * board, struct pos from)
{
    unset(board->trolls, from);
    board->numtrolls -= 1;
}

bool blockat(struct thudboard * board, struct pos pos)
{
    return get(board->blocks, pos);
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
    bits[pos.y] &= ~ bit(pos.x);
}

int countbits(uint16_t bits)
{
    /* from "Software Optimization Guide for AMD Athlon (tm) 64
     *       and Opteron (tm) Processors" */
    unsigned int w = bits - ((bits >> 1) & 0x55555555);
    unsigned int x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    unsigned int c = ((x + (x >> 4) & 0x0f0f0f0f) * 0x01010101) >> 24;
    return c;
}

int census(bitboard bits)
{
    int total = 0;
    for (int row=0; row < SIZE; ++row) total += countbits(bits[row]);
    return total;
}

bool hasneighbor(bitboard bits, struct pos pos)
{
    uint16_t mask = (uint16_t) 0xe000 >> (pos.x - 1);

    return ((bits[pos.y] & mask)
            || ((pos.y-1 >= 0) && (bits[pos.y - 1] & mask))
            || ((pos.y+1 < SIZE) && (bits[pos.y + 1] & mask)));
}

int countneighbors(bitboard bits, struct pos pos)
{
    uint16_t mask = (uint16_t) 0xe000 >> (pos.x - 1);
    int neighbors = countbits(bits[pos.y] & mask);
    if (pos.y-1 >= 0) neighbors += countbits(bits[pos.y - 1] & mask);
    if (pos.y+1 < SIZE) neighbors += countbits(bits[pos.y + 1] & mask);
    return neighbors;
}

struct pos nextpos(bitboard bits, ccrContParam)
{
    ccrBeginContext;
    int x,y;
    ccrEndContext(ctx);

    ccrBegin(ctx);
    for (ctx->y = 0; ctx->y < SIZE; ++ctx->y)
    {
        for (ctx->x = 0; ctx->x < SIZE; ++ctx->x)
        {
            if (! get(bits, pos(ctx->x,ctx->y))) continue;

            ccrReturn(pos(ctx->x,ctx->y));
        }
    }
    ccrFinishV; //???
}

struct pos pos(int x, int y)
{
    struct pos p = {x,y};
    return p;
}

bool inbounds(struct pos pos)
{
    return 0 <= pos.x && pos.x < SIZE
           && 0 <= pos.y && pos.y < SIZE;
}
