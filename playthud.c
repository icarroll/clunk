#include "ttable.h"
#include "thudlib.h"

#include <stdio.h>
#include <ctype.h>

enum {SEARCHSECS = 30};

struct move getmove(void);

struct thudboard board_data;
struct move move_data;

int main(int numargs, char * args[])
{
    struct thudboard * board = & board_data;
    struct move * move = & move_data;

    int memuse = 1024 * 1024 * (numargs > 1 ? strtol(args[1], NULL, 10)
                                            : sizeof(struct tableentry));

    setupgame(board, memuse);

    int c = toupper(getchar());

    if (getchar() != '\n') exit(EXIT_FAILURE);

    if (c == 'T') goto troll;
    else if (c != 'D') exit(EXIT_FAILURE);

    while (true)
    {
        * move = iterdeepen(board, SEARCHSECS);
        showmove(move);
        domoveupdatecapts(board, move);

troll:
        * move = getmove();
        if (! legalmove(board, move)) exit(EXIT_FAILURE);
        domoveupdatecapts(board, move);
    }
}

struct move getmove(void)
{
    struct move move;

    char * line = NULL;
    size_t size = 0;
    int _ = getline(& line, & size, stdin);

    char * cur = line;

    skipspace(& cur);
    if (toupper(* cur) == 'D') move.isdwarfmove = true;
    else if (toupper(* cur) == 'T') move.isdwarfmove = false;
    else exit(EXIT_FAILURE);
    cur += 1;

    bool valid;

    skipspace(& cur);
    valid = getpos(& cur, & move.from);
    if (! valid) exit(EXIT_FAILURE);

    skipspace(& cur);
    if (* cur++ != '-') exit(EXIT_FAILURE);

    skipspace(& cur);
    valid = getpos(& cur, & move.to);
    if (! valid) exit(EXIT_FAILURE);

    move.numcapts = 0;
    while (move.numcapts <= NUMDIRS)
    {
        skipspace(& cur);
        if (* cur == '\0' || * cur == '\n') break;

        skipspace(& cur);
        if (toupper(* cur++) != 'X') exit(EXIT_FAILURE);

        skipspace(& cur);
        valid = getpos(& cur, & move.capts[move.numcapts++]);
        if (! valid) exit(EXIT_FAILURE);
    }

    free(line);
    return move;
}
