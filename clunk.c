#include "ttable.h"
#include "thudlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <setjmp.h>
#include <readline/readline.h>
#include <readline/history.h>

enum {SEARCHSECS = 1};

void setupsides(void);
struct move humanmove(struct thudboard * board);
struct move computermove(struct thudboard * board);
struct move getmove(char * prompt);

enum {DWARF, TROLL};

typedef struct move movefunc_t(struct thudboard *);
movefunc_t * movefuncs[] = {computermove, computermove};

struct thudboard board_data;

int main(int numargs, char * args[])
{
    struct thudboard * board = & board_data;
    struct move move;

    int memuse = 1024 * 1024 * (numargs > 1 ? strtol(args[1], NULL, 10)
                                            : sizeof(struct tableentry));

    setupgame(board, memuse);

    using_history();
    setupsides();

    int moves_since_capt = 0;
    while (moves_since_capt <= 10)
    {
        putchar('\n');
        show(board);

        //time_t start = time(NULL);
        move = movefuncs[board->isdwarfturn ? DWARF : TROLL](board);
        //time_t elapsed = time(NULL) - start;
        //printf("Thinking took %d second%s.\n", elapsed, pl(elapsed));
        domoveupdatecapts(board, & move);

        if (move.numcapts) moves_since_capt = 0;
        else moves_since_capt += 1;
    }

    printf("game over, final score: dwarf=%d, troll=%d\n", board->trollscaptured*4, board->dwarfscaptured);
}

void setupsides(void)
{
    char * answer;
sideagain:
    answer = readline("Would you like to play Dwarf or Troll?\n");
    if (! answer) exit(EXIT_SUCCESS);

    char c = toupper(answer[0]);
    free(answer);

    if (c == 'T') movefuncs[TROLL] = humanmove;
    else if (c == 'D') movefuncs[DWARF] = humanmove;
    else if (c == 'B') movefuncs[TROLL] = movefuncs[DWARF] = humanmove;
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
    move = iterdeepen(board, SEARCHSECS);
    showmove(& move);

    return move;
}

struct move getmove(char * prompt)
{
    struct move move;

    char * line = NULL;
retry:
    line = readline(prompt);

    if (line)
    {
        if (* line) add_history(line);
        else
        {
            free(line);
            goto retry;
        }
    }
    else exit(EXIT_SUCCESS);

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

    return move;
}
