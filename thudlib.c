#include "thudlib.h"
#include "ttable.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <setjmp.h>

struct coord initpos = {-1,0};

int DX[NUMDIRS] = {0, 1, 1, 1, 0, -1, -1, -1};
int DY[NUMDIRS] = {-1, -1, 0, 1, 1, 1, 0, -1};

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
};

void setupgame(struct thudboard * board, long memuse)
{
    LOGF = stderr;
    inithash();
    initttable(memuse);
    setup(board);
}

struct move BROKEiterdeepen(struct thudboard * board, int searchtime)
{
    struct move bestmove;

    struct thudboard tempboard = * board;
    time_t stoptime = time(NULL) + searchtime;

    jmp_buf stopsearch;
    if (setjmp(stopsearch) == 0) for (int depth=1; true; depth += 1)
    {
        bestmove = zerowindow(& tempboard, depth, stoptime, stopsearch);

        //??? write to log?
        //printf("best move at depth %d: ", depth);
        //showmove(& bestmove);
    }

    return bestmove;
}

struct move iterdeepen(struct thudboard * board, int searchtime)
{
    struct move bestmove;

    struct thudboard tempboard = * board;
    time_t stoptime = time(NULL) + searchtime;

    jmp_buf stopsearch;
    if (setjmp(stopsearch) == 0) for (int depth=1; true; depth += 1)
    {
        struct move move;
        absearch(& tempboard, depth, FULL_WIDTH,
                 INT_MIN, INT_MAX, & move,
                 stoptime, stopsearch);
        bestmove = move;

        // log thoughts
        fprintf(LOGF, "best move at depth %d: ", depth);
        fshowmove(LOGF, & bestmove);
    }

    return bestmove;
}

struct move iterdeepenext(struct thudboard * board, int searchtime)
{
    struct move bestmove;

    struct thudboard tempboard = * board;
    time_t stoptime = time(NULL) + searchtime;

    jmp_buf stopsearch;
    if (setjmp(stopsearch) == 0) for (int depth=1; true; depth += 1)
    {
        struct move move;
        absearchext(& tempboard, depth, FULL_WIDTH,
                    INT_MIN, INT_MAX, & move,
                    stoptime, stopsearch);
        bestmove = move;

        // log thoughts
        fprintf(LOGF, "best move at depth %d: ", depth);
        fshowmove(LOGF, & bestmove);
    }

    return bestmove;
}

// search at least sdepth, then using up rest of stime if any
struct move searchdepthtime(struct thudboard * board, int sdepth, int stime) {
    struct move bestmove;

    struct thudboard tempboard = * board;
    time_t stoptime = time(NULL) + stime;

    for (int depth=1; depth <= sdepth; depth += 1)
    {
        struct move move;
        absearchext(& tempboard, depth, FULL_WIDTH,
                    INT_MIN, INT_MAX, & move,
                    0, NULL);
        bestmove = move;

        // log thoughts
        fprintf(LOGF, "best move at depth %d: ", depth);
        fshowmove(LOGF, & bestmove);
    }

    jmp_buf stopsearch;
    if (setjmp(stopsearch) == 0) for (int depth=sdepth+1; true; depth += 1)
    {
        struct move move;
        absearchext(& tempboard, depth, FULL_WIDTH,
                    INT_MIN, INT_MAX, & move,
                    stoptime, stopsearch);
        bestmove = move;

        // log thoughts
        fprintf(LOGF, "best move at depth %d: ", depth);
        fshowmove(LOGF, & bestmove);
    }

    return bestmove;
}

struct move beamiterdeepen(struct thudboard * board, int searchtime)
{
    struct move bestmove;

    struct thudboard tempboard = * board;
    time_t stoptime = time(NULL) + searchtime;

    for (int depth=1; depth <= 4; depth += 1)
    {
        struct move move;
        absearch(& tempboard, depth, FULL_WIDTH,
                 INT_MIN, INT_MAX, & move,
                 0, NULL);
        bestmove = move;

        //??? write to log?
        //printf("best move at depth %d: ", depth);
        //showmove(& bestmove);
    }

    jmp_buf stopsearch;
    if (setjmp(stopsearch) == 0) for (int depth=5; true; depth += 1)
    {
        struct move move;
        absearch(& tempboard, depth, 20,
                 INT_MIN, INT_MAX, & move,
                 stoptime, stopsearch);
        bestmove = move;

        /*
        //??? won't work without a ttable fix of some kind
        bestmove = zerowindow(& tempboard, depth, stoptime, stopsearch);
        */

        //??? write to log?
        //printf("best width 20 move at depth %d: ", depth);
        //showmove(& bestmove);
    }

    return bestmove;
}

struct move zerowindow(struct thudboard * board, int depth,
                       time_t stoptime, jmp_buf stopsearch)
{
    struct movelist list = allmoves(board);
    struct moveheap queue = heapof(board, & list);

    //??? should make sure move exists
    struct scoredmove best = popscored(& queue);

    domove(board, best.move);
    best.score = mtdf(board, depth, best.score, stoptime, stopsearch);
    undomove(board, best.move);

    while (queue.used > 0)
    {
        struct scoredmove temp = popscored(& queue);

        domove(board, temp.move);

        int result = absearch(board, depth, FULL_WIDTH,
                              best.score-1, best.score, NULL,
                              stoptime, stopsearch);

        if (result > best.score)
        {
            best = temp;
            best.score = _mtdf(board, depth,
                               result, result, INT_MAX,
                               stoptime, stopsearch);
        }

        undomove(board, temp.move);
    }

    return * best.move;
}

int mtdf(struct thudboard * board, int depth, int guess,
         time_t stoptime, jmp_buf stopsearch)
{
    return _mtdf(board, depth,
                 guess, INT_MIN, INT_MAX,
                 stoptime, stopsearch);
}

int _mtdf(struct thudboard * board, int depth,
          int guess, int min, int max,
          time_t stoptime, jmp_buf stopsearch)
{
    do
    {
        int beta = guess + (guess == min);

        guess = absearch(board, depth, FULL_WIDTH,
                         beta-1, beta, NULL,
                         stoptime, stopsearch);

        if (guess < beta) max = guess;
        else min = guess;
    } while (min < max);

    return guess;
}

static int max(int a, int b)
{
    return a > b ? a : b;
}

static int min(int a, int b)
{
    return a < b ? a : b;
}

int absearch(struct thudboard * board, int depth, int width,
             int trmin, int dwmax, struct move * bestmove,
             time_t stoptime, jmp_buf stopsearch)
{
    if (stoptime && time(NULL) > stoptime) longjmp(stopsearch, true);

    if (depth < 1) return evaluate(board);

    if (board->numtrolls < 1 || board->numdwarfs < 1) return evaluate(board);

    struct move * move;
    bool cutoff = false;

    int nextdepth = depth - 1;

    bool isdwarfturn = board->isdwarfturn;

    struct movelist list = allmoves(board);
    struct moveheap queue = heapof(board, & list);

    for (int i = width; i > 0 && queue.used > 0; --i)
    {
        move = pop(& queue);

        domove(board, move);

        int result;

        struct tableentry * entry = ttget(board->hash);
        if (entry && entry->depth >= nextdepth)
        {
            if (entry->min == entry->max) result = entry->min;
            else
            {
                if (entry->max <= trmin || entry->min >= dwmax)
                {
                    result = entry->scoreguess;
                }
                else
                {
                    int windlow = min(trmin, entry->min);
                    int windhigh = max(dwmax, entry->max);
                    result = absearch(board, nextdepth, width,
                                      windlow, windhigh, NULL,
                                      stoptime, stopsearch);

                    if (result < entry->min || result > entry->max)
                    {
                        goto search;
                    }

                    if (entry->depth == nextdepth)
                    {
                        entry->scoreguess = result;
                        if (result < windhigh) entry->max = result;
                        if (result > windlow) entry->min = result;
                    }
                }
            }
        }
        else
        {
search:
            result = absearch(board, nextdepth, width,
                              trmin, dwmax, NULL,
                              stoptime, stopsearch);

            int min, max;
            min = max = result;
            if (result < trmin) min = INT_MIN;
            else if (result > dwmax) max = INT_MAX;

            if (! entry) ttput((struct tableentry)
                    {board->hash, nextdepth, nextdepth, result, min, max});
        }

        // the only problem with not doing negamax
        if (isdwarfturn)
        {
            if (result < dwmax)
            {
                dwmax = result;
                if (bestmove) * bestmove = * move;
            }
        }
        else
        {
            if (result > trmin)
            {
                trmin = result;
                if (bestmove) * bestmove = * move;
            }
        }

        undomove(board, move);

        if (trmin >= dwmax)
        {
            cutoff = true;
            break;
        }
    }

    closeheap(& queue);
    closelist(& list);

    return (isdwarfturn ^ cutoff) ? dwmax : trmin;
}

int absearchext(struct thudboard * board, int depth, int width,
                int trmin, int dwmax, struct move * bestmove,
                time_t stoptime, jmp_buf stopsearch)
{
    if (stoptime && time(NULL) > stoptime) longjmp(stopsearch, true);

    if (depth < 1) return evaluate(board);

    struct move * move;
    bool cutoff = false;

    int nextdepth = depth - 1;

    bool isdwarfturn = board->isdwarfturn;

    struct movelist list = allmoves(board);
    struct moveheap queue = heapof(board, & list);

    for (int i = width; i > 0 && queue.used > 0; --i)
    {
        move = pop(& queue);

        // if moving a piece into threat, always search at least one more ply
        if (moveintothreat(board, move)) {
            nextdepth < SEARCH_EXT ? SEARCH_EXT : nextdepth;
        }

        domove(board, move);

        int result;

        struct tableentry * entry = ttget(board->hash);
        if (entry && entry->depth >= nextdepth)
        {
            if (entry->min == entry->max) result = entry->min;
            else
            {
                if (entry->max <= trmin || entry->min >= dwmax)
                {
                    result = entry->scoreguess;
                }
                else
                {
                    int windlow = min(trmin, entry->min);
                    int windhigh = max(dwmax, entry->max);
                    result = absearchext(board, nextdepth, width,
                                         windlow, windhigh, NULL,
                                         stoptime, stopsearch);

                    if (result < entry->min || result > entry->max)
                    {
                        goto search;
                    }

                    if (entry->depth == nextdepth)
                    {
                        entry->scoreguess = result;
                        if (result < windhigh) entry->max = result;
                        if (result > windlow) entry->min = result;
                    }
                }
            }
        }
        else
        {
search:
            result = absearchext(board, nextdepth, width,
                                 trmin, dwmax, NULL,
                                 stoptime, stopsearch);

            int min, max;
            min = max = result;
            if (result < trmin) min = INT_MIN;
            else if (result > dwmax) max = INT_MAX;

            if (! entry) ttput((struct tableentry)
                    {board->hash, nextdepth, nextdepth, result, min, max});
        }

        // the only problem with not doing negamax
        if (isdwarfturn)
        {
            if (result < dwmax)
            {
                dwmax = result;
                if (bestmove) * bestmove = * move;
            }
        }
        else
        {
            if (result > trmin)
            {
                trmin = result;
                if (bestmove) * bestmove = * move;
            }
        }

        undomove(board, move);

        if (trmin >= dwmax)
        {
            cutoff = true;
            break;
        }
    }

    closeheap(& queue);
    closelist(& list);

    return (isdwarfturn ^ cutoff) ? dwmax : trmin;
}

//TODO for now just check for dwarf moving next to troll
bool moveintothreat(struct thudboard * board, struct move * move) {
    return move->isdwarfmove && hasneighbor(board->trolls, move->to);
}

/*
int BROKEabsearch(struct thudboard * board, int depth, int width,
             int trmin, int dwmax, struct move * bestmove,
             time_t stoptime, jmp_buf stopsearch)
{
    if (stoptime && time(NULL) > stoptime) longjmp(stopsearch, true);

    if (depth < 1) return evaluate(board);

    struct move * move;
    bool cutoff = false;

    int nextdepth = depth - 1;

    bool isdwarfturn = board->isdwarfturn;

    struct genstate ctx;
    ctx.resume = NULL;

    struct movelist list;
    initlist(& list);

    struct moveheap queue;
    initheap(& queue);

    // prime the pump
    addmoves(10, board, & ctx, & list, & queue);

    for (int i = width; i > 0; --i)
    {
        if (ctx.resume) addmoves(10, board, & ctx, & list, & queue);
        if (queue.used == 0) break;
        move = pop(& queue);

        domove(board, move);

        int result;

        struct tableentry * entry = ttget(board->hash);
        if (entry && entry->depth >= nextdepth
            && ! (trmin < entry->score && entry->score <= entry->trmin
                  || entry->dwmax <= entry->score && entry->score < dwmax))
        {
            result = entry->score;
        }
        else
        {
            result = absearch(board, nextdepth, width,
                              trmin, dwmax, NULL,
                              stoptime, stopsearch);
            if (nextdepth > 0) ttput((struct tableentry)
                {board->hash, nextdepth, result, trmin, dwmax});
        }

        //??? simplify this
        if (isdwarfturn)
        {
            if (result < dwmax)
            {
                dwmax = result;
                if (bestmove) * bestmove = * move;
            }
        }
        else
        {
            if (result > trmin)
            {
                trmin = result;
                if (bestmove) * bestmove = * move;
            }
        }

        undomove(board, move);

        if (trmin >= dwmax)
        {
            cutoff = true;
            break;
        }
    }

    closeheap(& queue);
    closelist(& list);

    return (isdwarfturn ^ cutoff) ? dwmax : trmin;
}
*/

struct moveheap heapof(struct thudboard * board, struct movelist * list)
{
    bool isdwarfturn = board->isdwarfturn;

    struct moveheap heap;
    initheap(& heap);

    for (int i=0; i < list->used; ++i)
    {
        struct move * move = & (list->moves[i]);

        domove(board, move);
        int score = evaluate(board);
        if (isdwarfturn) score *= -1;
        undomove(board, move);

        insert(& heap, score, move);
    }

    return heap;
}

struct movelist allmoves(struct thudboard * board)
{
    return board->isdwarfturn ? alldwarfmoves(board)
                              : alltrollmoves(board);
}

void addmoves(int num, struct thudboard * board, struct genstate * ctx,
              struct movelist * list, struct moveheap * queue)
{
    bool isdwarfturn = board->isdwarfturn;

    struct move * move;

    for (int i=0; i < num; ++i)
    {
        move = next(list);
        * move = board->isdwarfturn ? nextdwarfplay(board, ctx)
                                    : nexttrollplay(board, ctx);
        if (! ctx->resume)
        {
            list->used -= 1;
            break;
        }

        domove(board, move);
        int score = evaluate(board);
        if (isdwarfturn) score *= -1;
        undomove(board, move);

        insert(queue, score, move);
    }
}

static bool occupied(struct thudboard * board, struct coord pos)
{
    return dwarfat(board, pos) || trollat(board, pos) || blockat(board, pos);
}

struct movelist alldwarfmoves(struct thudboard * board)
{
    struct movelist moves;
    initlist(& moves);

    // generate attacks
    struct coord from = initpos;
    while (true)
    {
        from = nextpos(board->dwarfs, from);
        if (from.x == -1) break;

        for (int dir = 0; dir < NUMDIRS; ++dir)
        {
            int dx = DX[dir];
            int dy = DY[dir];

            for (int dist = 1; dist < SIZE ; ++dist)
            {
                struct coord to = {from.x + dx * dist,
                                   from.y + dy * dist};
                if (! inbounds(to)) break;

                if (dwarfat(board, to) || blockat(board, to)) break;

                struct coord check = {from.x - dx * (dist-1),
                                      from.y - dy * (dist-1)};
                if (! inbounds(check) || ! dwarfat(board, check)) break;

                if (trollat(board, to))
                {
                    * next(& moves) = (struct move) {true, from, to, 1, {to}};
                    break;
                }
            }
        }
    }

    // generate moves
    from = initpos;
    while (true)
    {
        from = nextpos(board->dwarfs, from);
        if (from.x == -1) break;

        for (int dir = 0; dir < NUMDIRS; ++dir)
        {
            int dx = DX[dir];
            int dy = DY[dir];

            for (int dist = 1; dist < SIZE ; ++dist)
            {
                struct coord to = {from.x + dx * dist,
                                   from.y + dy * dist};
                if (! inbounds(to)) break;

                if (occupied(board, to)) break;

                * next(& moves) = (struct move) {true, from, to, 0, {}};
            }
        }
    }

    return moves;
}

struct move nextdwarfplay(struct thudboard * board, struct genstate * ctx)
{
    if (ctx->resume) goto * ctx->resume;

    ctx->move.isdwarfmove = true;

    // generate attacks
    ctx->move.numcapts = 1;
    ctx->move.from = initpos;
    while (true)
    {
        ctx->move.from = nextpos(board->dwarfs, ctx->move.from);
        if (ctx->move.from.x == -1) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DX[ctx->dir];
            ctx->dy = DY[ctx->dir];

            for (ctx->dist = 1; ctx->dist < SIZE ; ++ctx->dist)
            {
                ctx->move.to.x = ctx->move.from.x + ctx->dx * ctx->dist;
                ctx->move.to.y = ctx->move.from.y + ctx->dy * ctx->dist;
                if (! inbounds(ctx->move.to)) break;

                if (dwarfat(board, ctx->move.to)
                    || blockat(board, ctx->move.to)) break;

                struct coord check;
                check.x = ctx->move.from.x - ctx->dx * (ctx->dist-1);
                check.y = ctx->move.from.y - ctx->dy * (ctx->dist-1);
                if (! inbounds(check) || ! dwarfat(board, check)) break;

                if (trollat(board, ctx->move.to))
                {
                    ctx->move.capts[0] = ctx->move.to;

                    ctx->resume = && dwarfresume1;
                    return ctx->move;
dwarfresume1:
                    break;
                }
            }
        }
    }

    // generate moves
    ctx->move.numcapts = 0;
    ctx->move.from = initpos;
    while (true)
    {
        ctx->move.from = nextpos(board->dwarfs, ctx->move.from);
        if (ctx->move.from.x == -1) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DX[ctx->dir];
            ctx->dy = DY[ctx->dir];

            for (ctx->dist = 1; ctx->dist < SIZE ; ++ctx->dist)
            {
                ctx->move.to.x = ctx->move.from.x + ctx->dx * ctx->dist;
                ctx->move.to.y = ctx->move.from.y + ctx->dy * ctx->dist;
                if (! inbounds(ctx->move.to)) break;

                if (occupied(board, ctx->move.to)) break;

                ctx->resume = && dwarfresume2;
                return ctx->move;
dwarfresume2:
                continue;
            }
        }
    }

    ctx->resume = NULL;
}

void dodwarf(struct thudboard * board, struct move * move)
{
    if (move->numcapts > 0) capttroll(board, move->capts[0]);
    movedwarf(board, move->from, move->to);
    board->isdwarfturn = false;
    hashturn(board);
}

void undodwarf(struct thudboard * board, struct move * move)
{
    movedwarf(board, move->to, move->from);
    if (move->numcapts > 0) placetroll(board, move->capts[0]);
    board->isdwarfturn = true;
    hashturn(board);
}

struct movelist alltrollmoves(struct thudboard * board)
{
    struct movelist moves;
    initlist(& moves);

    // generate attacks
    struct coord from = initpos;
    while (true)
    {
        from = nextpos(board->trolls, from);
        if (from.x == -1) break;

        for (int dir = 0; dir < NUMDIRS; ++dir)
        {
            int dx = DX[dir];
            int dy = DY[dir];

            for (int dist = 1; dist < SIZE ; ++dist)
            {
                struct coord to = {from.x + dx * dist,
                                   from.y + dy * dist};
                if (! inbounds(to)) break;

                if (occupied(board, to)) break;

                struct coord check = {from.x - dx * (dist-1),
                                      from.y - dy * (dist-1)};
                if (! inbounds(check) || ! trollat(board, check)) break;

                if (hasneighbor(board->dwarfs, to))
                {
                    struct coord shovebuddy = {from.x - dx, from.y - dy};
                    if (dist == 1 && ! (inbounds(shovebuddy)
                                        && trollat(board, shovebuddy))) {
                        // can only capture one dwarf if not shoved
                        for (int i=0; i < NUMDIRS; ++i)
                        {
                            struct coord capt = {to.x + DX[i],
                                                 to.y + DY[i]};
                            if (inbounds(capt) && dwarfat(board, capt))
                            {
                                struct move * move = next(& moves);
                                * move = (struct move) {false, from, to, 0, {}};
                                move->capts[move->numcapts++] = capt;
                            }
                        }
                    }
                    else {
                        struct move * move = next(& moves);
                        * move = (struct move) {false, from, to, 0, {}};

                        for (int i=0; i < NUMDIRS; ++i)
                        {
                            struct coord capt = {to.x + DX[i],
                                                 to.y + DY[i]};
                            if (inbounds(capt) && dwarfat(board, capt))
                            {
                                move->capts[move->numcapts++] = capt;
                            }
                        }
                    }
                }
            }
        }
    }

    // generate moves
    from = initpos;
    while (true)
    {
        from = nextpos(board->trolls, from);
        if (from.x == -1) break;

        for (int dir = 0; dir < NUMDIRS; ++dir)
        {
            struct coord to = {from.x + DX[dir],
                               from.y + DY[dir]};

            if (! inbounds(to)) continue;

            if (occupied(board, to)) continue;

            * next(& moves) = (struct move) {false, from, to, 0, {}};
        }
    }

    return moves;
}

struct move nexttrollplay(struct thudboard * board, struct genstate * ctx)
{
    if (ctx->resume) goto * ctx->resume;

    ctx->move.isdwarfmove = false;

    // generate attacks
    ctx->move.from = initpos;
    while (true)
    {
        ctx->move.from = nextpos(board->trolls, ctx->move.from);
        if (ctx->move.from.x == -1) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DX[ctx->dir];
            ctx->dy = DY[ctx->dir];

            //TODO unshoved troll can only capture one dwarf
            for (ctx->dist = 1; ctx->dist < SIZE ; ++ctx->dist)
            {
                ctx->move.to.x = ctx->move.from.x + ctx->dx * ctx->dist;
                ctx->move.to.y = ctx->move.from.y + ctx->dy * ctx->dist;
                if (! inbounds(ctx->move.to)) break;

                if (occupied(board, ctx->move.to)) break;

                struct coord check;
                check.x = ctx->move.from.x - ctx->dx * (ctx->dist-1);
                check.y = ctx->move.from.y - ctx->dy * (ctx->dist-1);
                if (! inbounds(check) || ! trollat(board, check)) break;

                if (hasneighbor(board->dwarfs, ctx->move.to))
                {
                    ctx->move.numcapts = 0;
                    for (int i=0; i < NUMDIRS; ++i)
                    {
                        struct coord capt = {ctx->move.to.x + DX[i],
                                             ctx->move.to.y + DY[i]};
                        if (inbounds(capt) && dwarfat(board, capt))
                        {
                            ctx->move.capts[ctx->move.numcapts++] = capt;
                        }
                    }

                    ctx->resume = && trollresume1;
                    return ctx->move;
trollresume1:
                    continue;
                }
            }
        }
    }

    // generate moves
    ctx->move.numcapts = 0;
    ctx->move.from = initpos;
    while (true)
    {
        ctx->move.from = nextpos(board->trolls, ctx->move.from);
        if (ctx->move.from.x == -1) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DX[ctx->dir];
            ctx->dy = DY[ctx->dir];

            ctx->move.to.x = ctx->move.from.x + ctx->dx;
            ctx->move.to.y = ctx->move.from.y + ctx->dy;

            if (! inbounds(ctx->move.to)) continue;

            if (occupied(board, ctx->move.to)) break;

            ctx->resume = && trollresume2;
            return ctx->move;
trollresume2:
            continue;
        }
    }

    ctx->resume = NULL;
}

void dotroll(struct thudboard * board, struct move * move)
{
    captdwarfs(board, move->numcapts, move->capts);
    movetroll(board, move->from, move->to);
    board->isdwarfturn = true;
    hashturn(board);
}

void undotroll(struct thudboard * board, struct move * move)
{
    movetroll(board, move->to, move->from);
    placedwarfs(board, move->numcapts, move->capts);
    board->isdwarfturn = false;
    hashturn(board);
}

void erase(struct thudboard * board)
{
    memset(board, 0, sizeof(struct thudboard));
}

void setup(struct thudboard * board)
{
    erase(board);

    board->isdwarfturn = true;

    for (int y=0; y < SIZE; ++y)
    {
        for (int x=0; x < SIZE; ++x)
        {
            switch (toupper(stdlayout[y][x]))
            {
            case 'D':
                placedwarf(board, (struct coord) {x,y});
                break;
            case 'T':
                placetroll(board, (struct coord) {x,y});
                break;
            case '#':
                set(board->blocks, (struct coord) {x,y});
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

char * pl(int n)
{
    return n == 1 ? "" : "s";
}

void show(struct thudboard * board)
{
    puts("   A B C D E F G H J K L M N O P");
    for (int y=0; y < SIZE; ++y)
    {
        printf("%2d", y+1);
        for (int x=0; x < SIZE; ++x)
        {
            putchar(' ');

            if (dwarfat(board, (struct coord) {x,y})) putchar('d');
            else if (trollat(board, (struct coord) {x,y})) putchar('T');
            else if (blockat(board, (struct coord) {x,y})) putchar('#');
            else putchar('.');
        }
        putchar('\n');
    }

    putchar('\n');
    printf("%d Dwarf%s and %d Troll%s have been captured.\n",
           board->dwarfscaptured, pl(board->dwarfscaptured),
           board->trollscaptured, pl(board->trollscaptured));
    printf("It is the %s player's turn\n",
           board->isdwarfturn ? "Dwarf" : "Troll");

    fflush(stdout);
}

int eval_count = 0;

int evaluate(struct thudboard * board)
{
    eval_count += 1;
    struct tableentry * entry = ttget(board->hash);
    if (entry) return entry->scoreguess;
    else return heuristic(board);
}

int popcount16(uint16_t bits) {
    uint16_t count;
    count = (bits & 0x5555) + ((bits >> 1) & 0x5555);
    count = (count & 0x3333) + ((count >> 2) & 0x3333);
    count = (count & 0x0F0F) + ((count >> 4) & 0x0F0F);
    count = (count & 0x00FF) + ((count >> 8) & 0x00FF);
    return count;
}

int threatenedtrolls(struct thudboard * board) {
    bitboard threateneds;
    for (int n=0 ; n<SIZE ; n+=1) threateneds[n] = (uint16_t) 0;

    struct coord from = initpos;
    while (true)
    {
        from = nextpos(board->dwarfs, from);
        if (from.x == -1) break;

        for (int dir = 0; dir < NUMDIRS; ++dir)
        {
            int dx = DX[dir];
            int dy = DY[dir];

            for (int dist = 1; dist < SIZE ; ++dist)
            {
                struct coord to = {from.x + dx * dist,
                                   from.y + dy * dist};
                if (! inbounds(to)) break;

                if (dwarfat(board, to) || blockat(board, to)) break;

                struct coord check = {from.x - dx * (dist-1),
                                      from.y - dy * (dist-1)};
                if (! inbounds(check) || ! dwarfat(board, check)) break;

                if (trollat(board, to))
                {
                    set(threateneds, to);
                    break;
                }
            }
        }
    }

    int count = 0;
    for (int n=0 ; n<SIZE ; n+=1) {
        count += popcount16(threateneds[n]);
    }

    return count;
}

int threateneddwarfs(struct thudboard * board) {
    bitboard threateneds;
    for (int n=0 ; n<SIZE ; n+=1) threateneds[n] = (uint16_t) 0;

    struct coord from = initpos;
    while (true)
    {
        from = nextpos(board->trolls, from);
        if (from.x == -1) break;

        for (int dir = 0; dir < NUMDIRS; ++dir)
        {
            int dx = DX[dir];
            int dy = DY[dir];

            for (int dist = 1; dist < SIZE ; ++dist)
            {
                struct coord to = {from.x + dx * dist,
                                   from.y + dy * dist};
                if (! inbounds(to)) break;

                if (occupied(board, to)) break;

                struct coord check = {from.x - dx * (dist-1),
                                      from.y - dy * (dist-1)};
                if (! inbounds(check) || ! trollat(board, check)) break;

                if (hasneighbor(board->dwarfs, to))
                {
                    for (int i=0; i < NUMDIRS; ++i)
                    {
                        struct coord capt = {to.x + DX[i],
                                             to.y + DY[i]};
                        if (inbounds(capt) && dwarfat(board, capt))
                        {
                            set(threateneds, capt);
                        }
                    }
                }
            }
        }
    }

    int count = 0;
    for (int n=0 ; n<SIZE ; n+=1) {
        count += popcount16(threateneds[n]);
    }

    return count;
}

int area(bitboard bits) {
    bitrow_t smashed = 0;
    for (int ix=0 ; ix<SIZE ; ix+=1) smashed |= bits[ix];
    if (! smashed) return 0;

    int left;
    for (left=0 ; left<SIZE ; left+=1) if (HIGHBIT & (smashed << left)) break;

    int right;
    for (right=0 ; right<SIZE ; right+=1) if (0b10 & (smashed >> right)) break;

    int top;
    for (top=0 ; top<SIZE ; top+=1) if (bits[top] == 0) break;

    int bot;
    for (bot=SIZE-1 ; bot>=0 ; bot-=1) if (bits[bot] == 0) break;

    return (left - right) * (bot - top);
}

int heuristic(struct thudboard * board)
{
    int havetroll = 4000 * board->numtrolls;
    int threattroll = 0; //4000 * threatenedtrolls(board);
    int havedwarf = 1000 * board->numdwarfs;
    int threatdwarf = 0; //4000 * threateneddwarfs(board);
    int smol = 0; //15*15 - area(board->dwarfs);
    int clump = 10 * board->dwarfclump;
    int shake = ((double) random() / (double) RAND_MAX * 2.0 - 1.0) * 10.0;
    return (havetroll - threattroll) - (havedwarf - threatdwarf + smol + clump) + shake;
}

int score_game(struct thudboard * board)
{
    return 4000 * board->numtrolls
           - 1000 * board->numdwarfs;
}

bool legalmove(struct thudboard * board, struct move * move)
{
    // board and move must agree on whose turn it is
    if (board->isdwarfturn != move->isdwarfmove) return false;
    // move source must be in bounds
    else if (! inbounds(move->from)) return false;
    // move target must be in bounds
    else if (! inbounds(move->to)) return false;
    // check move details
    else if (move->isdwarfmove) return legaldwarfmove(board, move);
    else return legaltrollmove(board, move);
}

static int signum(int n)
{
    if (n > 0) return 1;
    else if (n < 0) return -1;
    else return 0;
}

bool legaldwarfmove(struct thudboard * board, struct move * move)
{
    // moving dwarf must be present
    if (! dwarfat(board, move->from)) return false;

    int dx = move->to.x - move->from.x;
    int dy = move->to.y - move->from.y;
    // can only move horizontal, vertical, or diagonal
    if (dx == 0)
    {
        // move length must be non-zero
        if (dy == 0) return false;
    }
    else if (dy != 0 && abs(dx) != abs(dy)) return false;

    int dist = max(abs(dx), abs(dy));
    int sx = signum(dx);
    int sy = signum(dy);

    // path must be clear
    for (int i=1; i < dist; ++i)
    {
        struct coord cur = {move->from.x + sx * i,
                            move->from.y + sy * i};
        if (occupied(board, cur)) return false;
    }

    if (move->numcapts)
    {
        // can only make one capture
        if (move->numcapts > 1) return false;
        // can only capture at move target
        else if (move->capts[0].x != move->to.x
                 || move->capts[0].y != move->to.y) return false;
        // target troll must be present
        else if (! trollat(board, move->capts[0])) return false;

        // can only capture using throw line
        for (int i=1; i < dist; ++i)
        {
            struct coord cur = {move->from.x - sx * i,
                                move->from.y - sy * i};
            if (! dwarfat(board, cur)) return false;
        }
    }
    // if not capturing, target must be clear
    else if (occupied(board, move->to)) return false;

    return true;
}

static bool adjacent(struct coord a, struct coord b)
{
    int dx = b.x - a.x;
    int dy = b.y - a.y;

    if (dx == 0 && dy == 0) return false;
    else if (abs(dx) > 1 || abs(dy) > 1) return false;
    else return true;
}

bool legaltrollmove(struct thudboard * board, struct move * move)
{
    // moving troll must be present
    if (! trollat(board, move->from)) return false;

    int dx = move->to.x - move->from.x;
    int dy = move->to.y - move->from.y;

    // move target must be clear
    if (occupied(board, move->to)) return false;

    if (move->numcapts)
    {
        int dist = max(abs(dx), abs(dy));
        int sx = signum(dx);
        int sy = signum(dy);

        // move length must be non-zero
        if (dist == 0) return false;

        // distant captures require shoving line
        for (int i=1; i < dist; ++i)
        {
            struct coord cur = {move->from.x - sx * i,
                                move->from.y - sy * i};
            if (! trollat(board, cur)) return false;
        }

        // check for buffer overrun
        if (move->numcapts > NUMDIRS) return false;

        for (int i=0; i < move->numcapts; ++i)
        {
            // capture targets must be unique
            for (int j=0; j < i; ++j)
            {
                if (move->capts[i].x == move->capts[j].x
                    && move->capts[i].y == move->capts[j].y) return false;
            }

            // capture targets must be in bounds
            if (! inbounds(move->capts[i])) return false;

            // capture targets must be adjacent
            if (! adjacent(move->to, move->capts[i])) return false;

            // target dwarfs must be present
            if (! dwarfat(board, move->capts[i])) return false;
        }
    }
    // if not shoving, can only move one space
    else if (! adjacent(move->from, move->to)) return false;

    return true;
}

void domove(struct thudboard * board, struct move * move)
{
    if (board->isdwarfturn) dodwarf(board, move);
    else dotroll(board, move);
}

void undomove(struct thudboard * board, struct move * move)
{
    if (board->isdwarfturn) undotroll(board, move);
    else undodwarf(board, move);
}

void domoveupdatecapts(struct thudboard * board, struct move * move)
{
    if (board->isdwarfturn)
    {
        dodwarf(board, move);
        board->trollscaptured += move->numcapts;
    }
    else
    {
        dotroll(board, move);
        board->dwarfscaptured += move->numcapts;
    }
}

//??? use strcspn
void skipspace(char ** input)
{
    for (; ** input == ' '; ++ * input);
}

int lettertocolumn(char c)
{
    c = toupper(c);

    if (c < 'A' || c == 'I' || c > 'Q') return -1;
    else return c - 'A' - (c > 'I');
}

bool getpos(char ** input, struct coord * pos)
{
    pos->x = lettertocolumn(** input);
    if (pos->x == -1) return false;
    * input += 1;

    int numchars;
    int numconvs = sscanf(* input, "%2d%n", & pos->y, & numchars);
    if (numconvs < 1) return false;
    pos->y -= 1;
    if (pos->y < 0 || pos->y >= SIZE) return false;

    * input += numchars;
    return true;
}

char * cols = "ABCDEFGHJKLMNOP";

void fshowpos(FILE * stream, struct coord pos)
{
    fprintf(stream, "%c%d", cols[pos.x], pos.y+1);
}

void showpos(struct coord pos)
{
    fshowpos(stdout, pos);
}

void fshowmove(FILE * stream, struct move * move)
{
    putc(move->isdwarfmove ? 'd' : 'T', stream);
    putc(' ', stream);
    fshowpos(stream, move->from);
    fputs("-", stream);
    fshowpos(stream, move->to);
    for (int i=0; i < move->numcapts; ++i)
    {
        fputs("x", stream);
        fshowpos(stream, move->capts[i]);
    }
    putc('\n', stream);
    fflush(stream);
}

void showmove(struct move * move)
{
    fshowmove(stdout, move);
}

void placedwarf(struct thudboard * board, struct coord to)
{
    board->dwarfclump += countneighbors(board->dwarfs, to);
    hashdwarf(board, to);
    set(board->dwarfs, to);
    board->numdwarfs += 1;
}

void placedwarfs(struct thudboard * board, int num, struct coord * tos)
{
    for (int i=0; i < num; ++i)
    {
        board->dwarfclump += countneighbors(board->dwarfs, tos[i]);
        hashdwarf(board, tos[i]);
        set(board->dwarfs, tos[i]);
    }
    board->numdwarfs += num;
}

void movedwarf(struct thudboard * board, struct coord from, struct coord to)
{
    unset(board->dwarfs, from);
    hashdwarf(board, from);
    board->dwarfclump -= countneighbors(board->dwarfs, from);
    board->dwarfclump += countneighbors(board->dwarfs, to);
    hashdwarf(board, to);
    set(board->dwarfs, to);
}

bool dwarfat(struct thudboard * board, struct coord pos)
{
    return get(board->dwarfs, pos);
}

void captdwarfs(struct thudboard * board, int num, struct coord * froms)
{
    for (int i=0; i < num; ++i)
    {
        unset(board->dwarfs, froms[i]);
        hashdwarf(board, froms[i]);
        board->dwarfclump -= countneighbors(board->dwarfs, froms[i]);
    }
    board->numdwarfs -= num;
}

void placetroll(struct thudboard * board, struct coord to)
{
    hashtroll(board, to);
    set(board->trolls, to);
    board->numtrolls += 1;
}

void movetroll(struct thudboard * board, struct coord from, struct coord to)
{
    unset(board->trolls, from);
    hashtroll(board, from);
    hashtroll(board, to);
    set(board->trolls, to);
}

bool trollat(struct thudboard * board, struct coord pos)
{
    return get(board->trolls, pos);
}

void capttroll(struct thudboard * board, struct coord from)
{
    unset(board->trolls, from);
    hashtroll(board, from);
    board->numtrolls -= 1;
}

bool blockat(struct thudboard * board, struct coord pos)
{
    return get(board->blocks, pos);
}

bitrow_t bit(int x)
{
    return (bitrow_t) HIGHBIT >> x;
}

bool get(bitboard bits, struct coord pos)
{
    return (bits[pos.y] & bit(pos.x)) != 0;
}

void set(bitboard bits, struct coord pos)
{
    bits[pos.y] |= bit(pos.x);
}

void unset(bitboard bits, struct coord pos)
{
    bits[pos.y] &= ~ bit(pos.x);
}

bool hasneighbor(bitboard bits, struct coord pos)
{
    for (int i=0; i < NUMDIRS; ++i)
    {
        struct coord neighbor = {pos.x + DX[i], pos.y + DY[i]};
        if (inbounds(neighbor) && get(bits, neighbor)) return true;
    }
    return false;
}

int countneighbors(bitboard bits, struct coord pos)
{
    int neighbors = 0;
    for (int i=0; i < NUMDIRS; ++i)
    {
        struct coord neighbor = {pos.x + DX[i], pos.y + DY[i]};
        if (inbounds(neighbor)) neighbors += get(bits, neighbor);
    }
    return neighbors;
}

struct coord nextpos(bitboard bits, struct coord cur)
{
    cur.x += 1;

    for (int i=cur.y*SIZE+cur.x; i < SIZE*SIZE; ++i)
    {
        struct coord pos = (struct coord) {i % SIZE, i / SIZE};
        if (get(bits, pos)) return pos;
    }

    return initpos;
}

void erasebits(bitboard bits)
{
    memset(bits, 0, sizeof(bitboard));
}

bool inbounds(struct coord pos)
{
    return 0 <= pos.x && pos.x < SIZE
           && 0 <= pos.y && pos.y < SIZE;
}
