#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

const int XSIZE = 15;
const int YSIZE = 15;

typedef uint16_t bitboard[16];

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
    bool isvalid;
    bool isdwarfmove;
    struct pos from;
    struct pos to;
};

const struct move nomove = {false, false, {-1,-1}, {-1,-1}};

struct movestate
{
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

struct movestate initdwarfplays(struct thudboard * board);
struct move nextdwarfplay(struct thudboard * board, struct movestate * status);
int dwarfsearch(struct thudboard * board, int depth, int alpha, int beta);
void undo(struct thudboard * board, struct move * play);

void die(char * msg);

void erase(struct thudboard * board);
void setup(struct thudboard * board);
void show(struct thudboard * board);

bool get(bitboard bits, int x, int y);
void set(bitboard bits, int x, int y);
void unset(bitboard bits, int x, int y);

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

    struct move bestmove = nomove;

    if (board->isdwarfturn)
    {
        struct movestate status = initdwarfplays(board);
        struct move play;

        while (true)
        {
            play = nextdwarfplay(board, & status);
            if (! play.isvalid) break;

            int result = dwarfsearch(board, depth-1, alpha, beta);
            if (result < beta)
            {
                beta = result;
                bestmove = play;
            }

            undo(board, & play);
        }
    }
    else
    {
    }

    return bestmove;
}

int dwarfsearch(struct thudboard * board, int depth, int alpha, int beta)
{
}

struct movestate initdwarfplays(struct thudboard * board)
{
}

struct move nextdwarfplay(struct thudboard * board, struct movestate * status)
{
}

void undo(struct thudboard * board, struct move * play)
{
}

void die(char * msg)
{
    fprintf(stderr, msg);
    exit(1);
}

void erase(struct thudboard * board)
{
    memset(board, 0, sizeof(struct thudboard));
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
                set(board->dwarfs, x, y);
                break;
            case 'T':
                set(board->trolls, x, y);
                break;
            case '#':
                set(board->blocks, x, y);
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

            if (get(board->dwarfs, x, y)) putchar('d');
            else if (get(board->trolls, x, y)) putchar('T');
            else if (get(board->blocks, x, y)) putchar('#');
            else putchar('.');
        }
        putchar('\n');
    }
    fflush(stdout);
}

uint16_t bit(int x)
{
    return (uint16_t) 0x8000 >> x;
}

bool get(bitboard bits, int x, int y)
{
    return (bits[y] & bit(x)) != 0;
}

void set(bitboard bits, int x, int y)
{
    bits[y] |= bit(x);
}

void unset(bitboard bits, int x, int y)
{
    bits[y] &= ~bit(x);
}
