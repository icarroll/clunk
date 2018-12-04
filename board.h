#ifndef BOARD_H
#define BOARD_H

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

    int captless;
};

#endif // BOARD_H
