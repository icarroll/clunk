#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <readline/readline.h>

enum {SIZE = 15};

typedef uint16_t bitrow_t;
typedef bitrow_t bitboard[SIZE];

typedef uint64_t hash_t;

struct thudboard
{
    bool isdwarfturn;

    bitboard dwarfs;
    bitboard trolls;
    bitboard blocks;

    int numdwarfs;
    int numtrolls;

    int dwarfclump;

    hash_t hash;

    int dwarfscaptured;
    int trollscaptured;
};

struct coord
{
    int x;
    int y;
};

struct coord initpos = {-1,0};

enum {NUMDIRS = 8};
int DX[NUMDIRS] = {0, 1, 1, 1, 0, -1, -1, -1};
int DY[NUMDIRS] = {-1, -1, 0, 1, 1, 1, 0, -1};

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
    hash_t hash;
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

void setupsides(void);
struct move humanmove(struct thudboard * board);
struct move computermove(struct thudboard * board);
struct move getmove(char * prompt);

struct move search(struct thudboard * board, int depth);

int dwarfsearch(struct thudboard * board, int depth, int trmin, int dwmax);
struct move nextdwarfplay(struct thudboard * board, struct genstate * ctx);
void dodwarf(struct thudboard * board, struct move * move);
void undodwarf(struct thudboard * board, struct move * move);

void placedwarf(struct thudboard * board, struct coord to);
void placedwarfs(struct thudboard * board, int num, struct coord * tos);
void movedwarf(struct thudboard * board, struct coord from, struct coord to);
bool dwarfat(struct thudboard * board, struct coord pos);
void captdwarfs(struct thudboard * board, int num, struct coord * froms);

int trollsearch(struct thudboard * board, int depth, int trmin, int dwmax);
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
void show(struct thudboard * board);
int evaluate(struct thudboard * board);
bool legalmove(struct thudboard * board, struct move * move);
void domoveforreal(struct thudboard * board, struct move * move);
void showpos(struct coord pos);
void showmove(struct move * move);

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
struct tableentry * ttget(hash_t hash);

typedef struct move movefunc_t(struct thudboard *);
movefunc_t * movefuncs[] = {computermove, computermove};
enum {DWARF, TROLL};

struct thudboard board_data;
int main(int numargs, char * args[])
{
    struct thudboard * board = & board_data;
    struct move move;

    inithash();
    setup(board);
    setupsides();

    while (true)
    {
        putchar('\n');
        show(board);

        move = movefuncs[board->isdwarfturn ? DWARF : TROLL](board);
        domoveforreal(board, & move);
    }
}

void setupsides(void)
{
    char * answer;
sideagain:
    answer = readline("Would you like to play Dwarf or Troll?\n");

    char c;
    if (answer) c = toupper(answer[0]);
    else goto sideagain;

    free(answer);

    if (c == 'T') movefuncs[TROLL] = humanmove;
    else if (c == 'D') movefuncs[DWARF] = humanmove;
    else if (c == 'N') /* computer vs computer */;
    else goto sideagain;
}

struct move humanmove(struct thudboard * board)
{
    struct move move;

moveagain:
    move = getmove("Your move?\n");
    if (! legalmove(board, & move))
    {
        printf("illegal move\n");
        goto moveagain;
    }

    return move;
}

struct move computermove(struct thudboard * board)
{
    struct move move;

    puts("Thinking...");
    fflush(stdout);
    move = search(board, 4);
    showmove(& move);

    return move;
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
    if (toupper(* cur) == 'D') move.isdwarfmove = true;
    else if (toupper(* cur) == 'T') move.isdwarfmove = false;
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
    while (move.numcapts <= NUMDIRS)
    {
        skipspace(& cur);
        if (* cur == '\0') break;

        skipspace(& cur);
        if (toupper(* cur++) != 'X')
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

struct move bestmove;
struct move search(struct thudboard * board, int depth)
{
    int trmin = INT_MIN;
    int dwmax = INT_MAX;

    struct move move;

    struct genstate ctx;
    ctx.resume = NULL;

    if (board->isdwarfturn)
    {
        while (true)
        {
            move = nextdwarfplay(board, & ctx);
            if (! ctx.resume) break;

            dodwarf(board, & move);

            int result;

            struct tableentry * entry = ttget(board->hash);
            if (entry && entry->depth >= depth-1)
            {
                result = entry->dwmax;
            }
            else
            {
                result = trollsearch(board, depth-1, trmin, dwmax);
                ttput(board, depth-1, trmin, result);
            }

            if (result < dwmax)
            {
                dwmax = result;
                bestmove = move;
            }

            undodwarf(board, & move);
        }
    }
    else
    {
        while (true)
        {
            move = nexttrollplay(board, & ctx);
            if (! ctx.resume) break;

            dotroll(board, & move);

            int result;

            struct tableentry * entry = ttget(board->hash);
            if (entry && entry->depth > depth)
            {
                result = entry->trmin;
            }
            else
            {
                result = dwarfsearch(board, depth-1, trmin, dwmax);
                ttput(board, depth-1, result, dwmax);
            }

            if (result > trmin)
            {
                trmin = result;
                bestmove = move;
            }

            undotroll(board, & move);
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
        struct move move = nextdwarfplay(board, & ctx);
        if (! ctx.resume) break;

        dodwarf(board, & move);

        int result;

        struct tableentry * entry = ttget(board->hash);
        if (entry && entry->depth > depth)
        {
            result = entry->dwmax;
        }
        else
        {
            result = trollsearch(board, depth-1, trmin, dwmax);
            ttput(board, depth-1, trmin, result);
        }

        if (result < dwmax) dwmax = result;

        undodwarf(board, & move);

        if (trmin >= dwmax) return trmin;
    }

    return dwmax;
}

/* ???
struct sortref
{
    int score;
    int index;
};

enum {MAXHEAP = 8};

struct heap
{
    int size;
    struct sortref refs[MAXHEAP];
    struct move moves[MAXHEAP];
};

void heappush(struct heap * heap, int score, struct move move)
{
    heap->size += 1;
    heap->refs[heap->size] = sortref(score, foo);
    heap->moves[heap->size] = move;
}

struct move heappop(struct heap * heap)
{
}
*/

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

    struct genstate ctx;
    ctx.resume = NULL;

    while (true)
    {
        struct move move = nexttrollplay(board, & ctx);
        if (! ctx.resume) break;

        dotroll(board, & move);

        int result;

        struct tableentry * entry = ttget(board->hash);
        if (entry && entry->depth > depth)
        {
            result = entry->trmin;
        }
        else
        {
            result = dwarfsearch(board, depth-1, trmin, dwmax);
            ttput(board, depth-1, result, dwmax);
        }

        if (result > trmin) trmin = result;

        undotroll(board, & move);

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

    for (int x=0; x < SIZE; ++x)
    {
        for (int y=0; y < SIZE; ++y)
        {
            switch (toupper(stdlayout[y][x]))
            {
            case 'D':
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

            if (dwarfat(board, coord(x,y))) putchar('d');
            else if (trollat(board, coord(x,y))) putchar('T');
            else if (blockat(board, coord(x,y))) putchar('#');
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

bool legaldwarfmove(struct thudboard * board, struct move * move);
bool legaltrollmove(struct thudboard * board, struct move * move);

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

int max(int a, int b)
{
    return a > b ? a : b;
}

int signum(int n)
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
        if (dwarfat(board, cur)
            || trollat(board, cur)
            || blockat(board, cur)) return false;
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
    else if (dwarfat(board, move->to)
             || trollat(board, move->to)
             || blockat(board, move->to)) return false;

    return true;
}

bool adjacent(struct coord a, struct coord b)
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
    if (dwarfat(board, move->to)
        || trollat(board, move->to)
        || blockat(board, move->to)) return false;

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

void domoveforreal(struct thudboard * board, struct move * move)
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
        struct coord pos = coord(i % SIZE, i / SIZE);
        if (get(bits, pos)) return pos;
    }

    return initpos;
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

hash_t turnhash;
hash_t dwarfhash[SIZE*SIZE];
hash_t trollhash[SIZE*SIZE];

hash_t randomhash(void)
{
    return (hash_t) random() << 32 ^ random();
}

void inithash(void)
{
    turnhash = randomhash();

    for (int i=0; i < SIZE*SIZE; ++i)
    {
        dwarfhash[i] = randomhash();
        trollhash[i] = randomhash();
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

int ttindex(hash_t hash)
{
    return hash % TTABLESIZE;
}

void ttput(struct thudboard * board, int depth, int trmin, int dwmax)
{
    int index = ttindex(board->hash);

    struct tableentry temp =
    {
        board->hash,
        depth,
        trmin,
        dwmax,
    };
    ttable[index] = temp;
}

struct tableentry * ttget(hash_t hash)
{
    int index = ttindex(hash);

    if (ttable[index].hash == hash) return & (ttable[index]);
    else return NULL;
}
