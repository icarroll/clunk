#ifndef MOVE_H
#define MOVE_H

#include <stdbool.h>

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

#endif // MOVE_H
