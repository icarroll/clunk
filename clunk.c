#include "ttable.h"
#include "thudlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <readline/readline.h>

enum {DEPTH = 4};

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

    setupsides();

    while (true)
    {
        putchar('\n');
        show(board);

        time_t start = time(NULL);
        move = movefuncs[board->isdwarfturn ? DWARF : TROLL](board);
        time_t elapsed = time(NULL) - start;
        printf("Thinking took %d second%s.\n", elapsed, pl(elapsed));
        domoveforreal(board, & move);
    }
}

void setupsides(void)
{
    char * answer;
sideagain:
    answer = readline("Would you like to play Dwarf or Troll?\n");
    if (! answer) goto sideagain;

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
    move = search(board, DEPTH);
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
    if (! line) goto retry;

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
