#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

const int XSIZE = 15;
const int YSIZE = 15;

typedef uint16_t bitboard[16];

struct thudboard
{
    bitboard dwarfs;
    bitboard trolls;
    bitboard blocks;
    bool dwarfturn;
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

    board->dwarfturn = true;

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
                die("bad layout\n");
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

int frob(int x, int y)
{
    return y;
}

uint16_t munge(int x, int y)
{
    return (uint16_t) 0x8000 >> x;
}

bool get(bitboard bits, int x, int y)
{
    return (bits[frob(x,y)] & munge(x,y)) != 0;
}

void set(bitboard bits, int x, int y)
{
    bits[frob(x,y)] |= munge(x,y);
}

void unset(bitboard bits, int x, int y)
{
    bits[frob(x,y)] &= ~munge(x,y);
}
