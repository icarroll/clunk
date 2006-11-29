#include "thudlib.h"
#include "ttable.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

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

void setupgame(struct thudboard * board, int memuse)
{
    inithash();
    initttable(memuse);
    setup(board);
}

struct move search(struct thudboard * board, int depth)
{
    int trmin = INT_MIN;
    int dwmax = INT_MAX;

    struct move bestmove;
    struct move * move;

    if (board->isdwarfturn)
    {
        struct movelist list = alldwarfmoves(board);
        struct moveheap queue = heapof(board, & list);

        while (queue.used > 0)
        {
            move = pop(& queue);

            dodwarf(board, move);

            int result;

            struct tableentry * entry = ttget(board->hash);
            if (entry && entry->depth >= depth-1)
            {
                result = entry->score;
            }
            else
            {
                result = trollsearch(board, depth-1, trmin, dwmax);
                ttput((struct tableentry)
                      {board->hash, depth-1, result, trmin, dwmax});
            }

            if (result < dwmax)
            {
                dwmax = result;
                bestmove = * move;
            }

            undodwarf(board, move);
        }

        closeheap(& queue);
        closelist(& list);
    }
    else
    {
        struct movelist list = alltrollmoves(board);
        struct moveheap queue = heapof(board, & list);

        while (queue.used > 0)
        {
            move = pop(& queue);

            dotroll(board, move);

            int result;

            struct tableentry * entry = ttget(board->hash);
            if (entry && entry->depth > depth)
            {
                result = entry->score;
            }
            else
            {
                result = dwarfsearch(board, depth-1, trmin, dwmax);
                ttput((struct tableentry)
                      {board->hash, depth-1, result, trmin, dwmax});
            }

            if (result > trmin)
            {
                trmin = result;
                bestmove = * move;
            }

            undotroll(board, move);
        }

        closeheap(& queue);
        closelist(& list);
    }

    return bestmove;
}

struct moveheap heapof(struct thudboard * board, struct movelist * list)
{
    struct moveheap heap;
    initheap(& heap);

    for (int i=0; i < list->used; ++i)
    {
        struct move * move = & (list->moves[i]);

        domove(board, move);
        int score = evaluate(board);
        if (board->isdwarfturn) score *= -1;
        undomove(board, move);

        insert(& heap, score, move);
    }

    return heap;
}

static bool occupied(struct thudboard * board, struct coord pos)
{
    return dwarfat(board, pos) || trollat(board, pos) || blockat(board, pos);
}

int dwarfsearch(struct thudboard * board, int depth, int trmin, int dwmax)
{
    if (depth < 1) return evaluate(board);

    struct movelist list = alldwarfmoves(board);
    struct moveheap queue = heapof(board, & list);

    while (queue.used > 0)
    {
        struct move * move = pop(& queue);

        dodwarf(board, move);

        int result;

        struct tableentry * entry = ttget(board->hash);
        if (entry && entry->depth > depth)
        {
            result = entry->score;
        }
        else
        {
            result = trollsearch(board, depth-1, trmin, dwmax);
            ttput((struct tableentry)
                  {board->hash, depth-1, result, trmin, dwmax});
        }

        if (result < dwmax) dwmax = result;

        undodwarf(board, move);

        if (trmin >= dwmax)
        {
            closeheap(& queue);
            closelist(& list);
            return trmin;
        }
    }

    closeheap(& queue);
    closelist(& list);
    return dwmax;
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
                    * next(& moves) = (struct move) {true, from, to, 1, to};
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

                * next(& moves) = (struct move) {true, from, to, 0};
            }
        }
    }

    return moves;
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

int trollsearch(struct thudboard * board, int depth, int trmin, int dwmax)
{
    if (depth < 1) return evaluate(board);

    struct movelist list = alltrollmoves(board);
    struct moveheap queue = heapof(board, & list);

    while (queue.used > 0)
    {
        struct move * move = pop(& queue);

        dotroll(board, move);

        int result;

        struct tableentry * entry = ttget(board->hash);
        if (entry && entry->depth > depth)
        {
            result = entry->score;
        }
        else
        {
            result = dwarfsearch(board, depth-1, trmin, dwmax);
            ttput((struct tableentry)
                  {board->hash, depth-1, result, trmin, dwmax});
        }

        if (result > trmin) trmin = result;

        undotroll(board, move);

        if (trmin >= dwmax)
        {
            closeheap(& queue);
            closelist(& list);
            return dwmax;
        }
    }

    closeheap(& queue);
    closelist(& list);
    return trmin;
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
                    struct move * move = next(& moves);
                    * move = (struct move) {false, from, to, 0};

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

            if (occupied(board, to)) break;

            * next(& moves) = (struct move) {false, from, to, 0};
        }
    }

    return moves;
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

int evaluate(struct thudboard * board)
{
    return 4000 * board->numtrolls
           - 1000 * board->numdwarfs
           - board->dwarfclump;
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

static int max(int a, int b)
{
    return a > b ? a : b;
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

char * cols = "ABCDEFGHJKLMNOP";

void showpos(struct coord pos)
{
    printf("%c%d", cols[pos.x], pos.y+1);
}

void showmove(struct move * move)
{
    putchar(move->isdwarfmove ? 'd' : 'T');
    putchar(' ');
    showpos(move->from);
    fputs(" - ", stdout);
    showpos(move->to);
    for (int i=0; i < move->numcapts; ++i)
    {
        fputs(" x ", stdout);
        showpos(move->capts[i]);
    }
    putchar('\n');
    fflush(stdout);
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

enum {HIGHBIT = 1 << (SIZE - 1)};

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
