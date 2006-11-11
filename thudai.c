#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <readline/readline.h>

enum {SIZE = 15};

typedef uint16_t bitboard[SIZE];

struct thudboard
{
    bool isdwarfturn;

    bitboard dwarfs;
    bitboard trolls;
    bitboard blocks;

    int numdwarfs;
    int numtrolls;

    int dwarfclump;

    uint64_t hash;
};

struct coord
{
    int x;
    int y;
};

struct coord initpos = {-1,0};

enum {NUMDIRS = 8};
int DXS[] = {0, 1, 1, 1, 0, -1, -1, -1};
int DYS[] = {-1, -1, 0, 1, 1, 1, 0, -1};

struct move
{
    bool isdwarfmove;
    struct coord from;
    struct coord to;
    int numcapts;
    struct coord capts[NUMDIRS];
};

struct genstate
{
    void * resume;
    int dx,dy;
    int dir;
    int dist;
    struct move move;
};

struct tableentry
{
    uint64_t hash;
    int numdwarfs;
    int numtrolls;
    int depth;
    int trmin;
    int dwmax;
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
};

struct move search(struct thudboard * board, int depth);

int dwarfsearch(struct thudboard * board, int depth, int trmin, int dwmax);
struct move nextdwarfplay(struct thudboard * board, struct genstate * ctx);
void dodwarf(struct thudboard * board, struct move * play);
void undodwarf(struct thudboard * board, struct move * play);

void placedwarf(struct thudboard * board, struct coord to);
void placedwarfs(struct thudboard * board, int num, struct coord * tos);
void movedwarf(struct thudboard * board, struct coord from, struct coord to);
bool dwarfat(struct thudboard * board, struct coord pos);
void captdwarfs(struct thudboard * board, int num, struct coord * froms);

int trollsearch(struct thudboard * board, int depth, int trmin, int dwmax);
struct move nexttrollplay(struct thudboard * board, struct genstate * ctx);
void dotroll(struct thudboard * board, struct move * play);
void undotroll(struct thudboard * board, struct move * play);

void placetroll(struct thudboard * board, struct coord to);
void movetroll(struct thudboard * board, struct coord from, struct coord to);
bool trollat(struct thudboard * board, struct coord pos);
void capttroll(struct thudboard * board, struct coord from);

bool blockat(struct thudboard * board, struct coord pos);

void erase(struct thudboard * board);
void setup(struct thudboard * board);
void show(struct thudboard * board);
int evaluate(struct thudboard * board);
bool legalmove(struct thudboard * board, struct move * play);
void domove(struct thudboard * board, struct move * play);
void showpos(struct coord pos);
void showmove(struct move * play);
struct move getmove(char * prompt);

bool get(bitboard bits, struct coord pos);
void set(bitboard bits, struct coord pos);
void unset(bitboard bits, struct coord pos);
bool hasneighbor(bitboard bits, struct coord pos);
int countneighbors(bitboard bits, struct coord pos);
struct coord nextpos(bitboard bits, struct coord);

struct coord coord(int x, int y);
bool inbounds(struct coord);

void inithash(void);
void hashturn(struct thudboard * board);
void hashdwarf(struct thudboard * board, struct coord to);
void hashtroll(struct thudboard * board, struct coord to);

void ttput(struct thudboard * board, int depth, int trmin, int dwmax);
struct tableentry * ttget(uint64_t hash);

struct thudboard board_data;
int main(int numargs, char * args[])
{
    struct thudboard * board = & board_data;
    struct move play;

    inithash();
    setup(board);

    while (true)
    {
        /*
        show(board);

        play = getmove("your move:\n");
        if (! legalmove(board, & play)) printf("illegal move\n");
        putchar('\n');
        domove(board, & play);
        */

        show(board);

        puts("computer move:");
        play = search(board, 4);
        showmove(& play);
        putchar('\n');
        domove(board, & play);
    }
}

struct move bestmove;
struct move search(struct thudboard * board, int depth)
{
    int trmin = INT_MIN;
    int dwmax = INT_MAX;

    struct move play;

    struct genstate ctx;
    ctx.resume = NULL;

    if (board->isdwarfturn)
    {
        while (true)
        {
            play = nextdwarfplay(board, & ctx);
            if (! ctx.resume) break;

            dodwarf(board, & play);

            int result = trollsearch(board, depth-1, trmin, dwmax);
            if (result < dwmax)
            {
                dwmax = result;
                bestmove = play;
            }

            undodwarf(board, & play);
        }
    }
    else
    {
        while (true)
        {
            play = nexttrollplay(board, & ctx);
            if (! ctx.resume) break;

            dotroll(board, & play);

            int result = dwarfsearch(board, depth-1, trmin, dwmax);
            if (result > trmin)
            {
                trmin = result;
                bestmove = play;
            }

            undotroll(board, & play);
        }
    }

    return bestmove;
}

int dwarfsearch(struct thudboard * board, int depth, int trmin, int dwmax)
{
    if (depth < 1) return evaluate(board);

    struct genstate ctx;
    ctx.resume = NULL;

    while (true)
    {
        struct move play = nextdwarfplay(board, & ctx);
        if (! ctx.resume) break;

        dodwarf(board, & play);

        int result = trollsearch(board, depth-1, trmin, dwmax);
        if (result < dwmax) dwmax = result;

        undodwarf(board, & play);

        if (trmin >= dwmax) return trmin;
    }

    return dwmax;
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
        if (ctx->move.from.y >= SIZE) break;

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
        if (ctx->move.from.y >= SIZE) break;

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

                ctx->resume = && dwarfresume2;
                return ctx->move;
dwarfresume2:
                continue;
            }
        }
    }

    ctx->resume = NULL;
}

void dodwarf(struct thudboard * board, struct move * play)
{
    if (play->numcapts > 0) capttroll(board, play->capts[0]);
    movedwarf(board, play->from, play->to);
    board->isdwarfturn = false;
    hashturn(board);
}

void undodwarf(struct thudboard * board, struct move * play)
{
    movedwarf(board, play->to, play->from);
    if (play->numcapts > 0) placetroll(board, play->capts[0]);
    board->isdwarfturn = true;
    hashturn(board);
}

int trollsearch(struct thudboard * board, int depth, int trmin, int dwmax)
{
    if (depth < 1) return evaluate(board);

    struct genstate ctx;
    ctx.resume = NULL;

    while (true)
    {
        struct move play = nexttrollplay(board, & ctx);
        if (! ctx.resume) break;

        dotroll(board, & play);

        int result = dwarfsearch(board, depth-1, trmin, dwmax);
        if (result > trmin) trmin = result;

        undotroll(board, & play);

        if (trmin >= dwmax) return dwmax;
    }

    return trmin;
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
        if (ctx->move.from.y >= SIZE) break;

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

                struct coord check;
                check.x = ctx->move.from.x - ctx->dx * (ctx->dist-1);
                check.y = ctx->move.from.y - ctx->dy * (ctx->dist-1);
                if (! inbounds(check) || ! trollat(board, check)) break;

                if (hasneighbor(board->dwarfs, ctx->move.to))
                {
                    ctx->move.numcapts = 0;
                    for (int i=0; i < NUMDIRS; ++i)
                    {
                        struct coord capt = {ctx->move.to.x + DXS[i],
                                           ctx->move.to.y + DYS[i]};
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
        if (ctx->move.from.y >= SIZE) break;

        for (ctx->dir = 0; ctx->dir < NUMDIRS; ++ctx->dir)
        {
            ctx->dx = DXS[ctx->dir];
            ctx->dy = DYS[ctx->dir];

            ctx->move.to.x = ctx->move.from.x + ctx->dx;
            ctx->move.to.y = ctx->move.from.y + ctx->dy;

            if (! inbounds(ctx->move.to)) continue;

            if (dwarfat(board, ctx->move.to)
                || trollat(board, ctx->move.to)
                || blockat(board, ctx->move.to)) continue;

            ctx->resume = && trollresume2;
            return ctx->move;
trollresume2:
            continue;
        }
    }

    ctx->resume = NULL;
}

void dotroll(struct thudboard * board, struct move * play)
{
    captdwarfs(board, play->numcapts, play->capts);
    movetroll(board, play->from, play->to);
    board->isdwarfturn = true;
    hashturn(board);
}

void undotroll(struct thudboard * board, struct move * play)
{
    movetroll(board, play->to, play->from);
    placedwarfs(board, play->numcapts, play->capts);
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

    board->numdwarfs = 0;
    board->numtrolls = 0;

    for (int x=0; x < SIZE; ++x)
    {
        for (int y=0; y < SIZE; ++y)
        {
            switch (stdlayout[y][x])
            {
            case 'd':
                placedwarf(board, coord(x,y));
                break;
            case 'T':
                placetroll(board, coord(x,y));
                break;
            case '#':
                set(board->blocks, coord(x,y));
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
    puts("   A B C D E F G H J K L M N P Q");
    for (int y=0; y < SIZE; ++y)
    {
        printf("%2d", y+1);
        for (int x=0; x < SIZE; ++x)
        {
            putchar(' ');

            if (get(board->dwarfs, coord(x,y))) putchar('d');
            else if (get(board->trolls, coord(x,y))) putchar('T');
            else if (get(board->blocks, coord(x,y))) putchar('#');
            else putchar('.');
        }
        putchar('\n');
    }
    printf("%s turn\n\n", board->isdwarfturn ? "dwarf" : "troll");
    fflush(stdout);
}

int evaluate(struct thudboard * board)
{
    return 4000 * board->numtrolls
           - 1000 * board->numdwarfs
           - board->dwarfclump;
}

bool legalmove(struct thudboard * board, struct move * play)
{
    if (board->isdwarfturn != play->isdwarfmove) return false;

    // stupid c
    uint16_t * we = play->isdwarfmove ? board->dwarfs : board->trolls;
    uint16_t * they = play->isdwarfmove ? board->trolls : board->dwarfs;

    if (! get(we, play->from)) return false;

    if (get(board->dwarfs, play->to)
        || get(board->trolls, play->to)
        || get(board->blocks, play->to)) return false;

    for (int i=0; i < play->numcapts; ++i)
    {
        if (! get(they, play->capts[i])) return false;
    }

    //??? complete validation

    return true;
}

void domove(struct thudboard * board, struct move * play)
{
    if (board->isdwarfturn) dodwarf(board, play);
    else dotroll(board, play);
}

char * cols = "ABCDEFGHJKLMNPQ";

void showpos(struct coord pos)
{
    printf("%c%d", cols[pos.x], pos.y+1);
}

void showmove(struct move * play)
{
    putchar(play->isdwarfmove ? 'd' : 'T');
    putchar(' ');
    showpos(play->from);
    fputs(" - ", stdout);
    showpos(play->to);
    for (int i=0; i < play->numcapts; ++i)
    {
        fputs(" x ", stdout);
        showpos(play->capts[i]);
    }
    putchar('\n');
    fflush(stdout);
}

void skipspace(char ** input)
{
    for (; ** input == ' '; ++ * input);
}

int lettertocolumn(char c)
{
    if (c < 'A' || c == 'I' || c == 'O' || c > 'Q') return -1;
    else return c - 'A' - (c > 'I') - (c > 'O');
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

struct move getmove(char * prompt)
{
    struct move move;

    char * line = NULL;
retry:
    if (line) free(line);
    line = readline(prompt);
    if (! line)
    {
        printf("blank line\n");
        goto retry;
    }

    char * cur = line;

    skipspace(& cur);
    if (* cur == 'd') move.isdwarfmove = true;
    else if (* cur == 'T') move.isdwarfmove = false;
    else
    {
        printf("bad side %c\n", * cur);
        goto retry;
    }
    cur += 1;

    bool valid;

    skipspace(& cur);
    valid = getpos(& cur, & move.from);
    if (! valid)
    {
        printf("bad from\n");
        goto retry;
    }

    skipspace(& cur);
    if (* cur++ != '-')
    {
        printf("missing -\n");
        goto retry;
    }

    skipspace(& cur);
    valid = getpos(& cur, & move.to);
    if (! valid)
    {
        printf("bad to\n");
        goto retry;
    }

    move.numcapts = 0;
    while (move.numcapts < NUMDIRS)
    {
        skipspace(& cur);
        if (* cur == '\0') break;

        skipspace(& cur);
        if (* cur++ != 'x')
        {
            printf("missing x\n");
            goto retry;
        }

        skipspace(& cur);
        valid = getpos(& cur, & move.capts[move.numcapts++]);
        if (! valid)
        {
            printf("bad capt\n");
            goto retry;
        }
    }

    free(line);
    return move;
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

uint16_t bit(int x)
{
    return (uint16_t) 0x8000 >> x;
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

/*
int countbits(uint16_t bits)
{
    // from "Software Optimization Guide for AMD Athlon (tm) 64
    //       and Opteron (tm) Processors"
    unsigned int w = bits - ((bits >> 1) & 0x55555555);
    unsigned int x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    unsigned int c = ((x + (x >> 4) & 0x0f0f0f0f) * 0x01010101) >> 24;
    return c;
}
*/

bool hasneighbor(bitboard bits, struct coord pos)
{
    for (int i=0; i < NUMDIRS; ++i)
    {
        struct coord neighbor = {pos.x + DXS[i], pos.y + DYS[i]};
        if (inbounds(neighbor) && get(bits, neighbor)) return true;
    }
    return false;
}

int countneighbors(bitboard bits, struct coord pos)
{
    int neighbors = 0;
    for (int i=0; i < NUMDIRS; ++i)
    {
        struct coord neighbor = {pos.x + DXS[i], pos.y + DYS[i]};
        if (inbounds(neighbor)) neighbors += get(bits, neighbor);
    }
    return neighbors;
}

struct coord nextpos(bitboard bits, struct coord cur)
{
    cur.x += 1;

    for (int i=cur.y*SIZE+cur.x; i < SIZE*SIZE; ++i)
    {
        struct coord pos = coord(i % SIZE, i / SIZE);
        if (get(bits, pos)) return pos;
    }
}

struct coord coord(int x, int y)
{
    struct coord p = {x,y};
    return p;
}

bool inbounds(struct coord pos)
{
    return 0 <= pos.x && pos.x < SIZE
           && 0 <= pos.y && pos.y < SIZE;
}

uint64_t turnhash;
uint64_t dwarfhash[SIZE*SIZE];
uint64_t trollhash[SIZE*SIZE];

uint64_t random64(void)
{
    return (uint64_t) random() << 32 | random();
}

void inithash(void)
{
    turnhash = random64();

    for (int i=0; i < SIZE*SIZE; ++i)
    {
        dwarfhash[i] = random64();
        trollhash[i] = random64();
    }
}

void hashturn(struct thudboard * board)
{
    board->hash ^= turnhash;
}

void hashdwarf(struct thudboard * board, struct coord to)
{
    board->hash ^= dwarfhash[to.y*SIZE + to.x];
}

void hashtroll(struct thudboard * board, struct coord to)
{
    board->hash ^= trollhash[to.y*SIZE + to.x];
}

enum
{
    MEMUSE = 1024 * 1024 * 1024,
    TTABLESIZE = MEMUSE / sizeof(struct tableentry),
};

struct tableentry ttable[TTABLESIZE];

int ttindex(uint64_t hash)
{
    return hash % TTABLESIZE;
}

void ttput(struct thudboard * board, int depth, int trmin, int dwmax)
{
    int index = ttindex(board->hash);

    struct tableentry temp =
    {
        board->hash,
        board->numdwarfs,
        board->numtrolls,
        depth,
        trmin,
        dwmax,
    };
    ttable[index] = temp;
}

struct tableentry * ttget(uint64_t hash)
{
    int index = ttindex(hash);

    if (ttable[index].hash == hash) return & (ttable[index]);
    else return NULL;
}
