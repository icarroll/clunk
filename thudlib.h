#ifndef THUDLIB_H
#define THUDLIB_H

#include <stdint.h>
#include <stdbool.h>

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

struct thudboard board_data;

struct coord
{
    int x;
    int y;
};

enum {NUMDIRS = 8};

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

static char * BOOKFILENAME = "thud.book";

struct tableentry
{
    hash_t hash;
    int depth;
    int trmin;
    int dwmax;
};

void setupgame(struct thudboard * board, int memuse);

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
char * pl(int n);
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

typedef struct move movefunc_t(struct thudboard *);
enum {DWARF, TROLL};

#endif //THUDLIB_H
