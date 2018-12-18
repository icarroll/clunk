#include <stdio.h>
#include <unistd.h>

#include "thudlib.h"

extern struct coord initpos;

enum {SEARCHSECS = 1};
enum {SEARCHDEPTH = 3};

struct thudboard board_data;

int main(int numargs, char * args[]) {
    struct movelist moves;
    initlist(& moves);

    struct thudboard * board = & board_data;
    struct move move;

    long memuse = 1l * 1024 * 1024 * 1024;
    setupgame(board, memuse);
    LOGF = fopen("/dev/null", "w");

    int starttime = time(NULL);
    int pid = getpid();
    srandom(starttime ^ pid);

    int moves_since_capt = 0;
    while (board->numdwarfs && board->numtrolls && moves_since_capt <= DRAW_DEADLINE)
    {
        //move = iterdeepen(board, SEARCHSECS);
        absearch(board, SEARCHDEPTH, FULL_WIDTH, INT_MIN, INT_MAX, & move, 0, NULL);
        * next(& moves) = move;
        showmove(& move);
        domoveupdatecapts(board, & move);

        if (move.numcapts) moves_since_capt = 0;
        else moves_since_capt += 1;
    }

    int finalscore = score_game(board);
    printf("%d\n", finalscore);

    // write out data file
    char filename[65536];
    sprintf(filename, "run-depth%d-pid%d-%d", SEARCHDEPTH, pid, starttime);
    FILE * out = fopen(filename, "w");

    setup(board);

    bitboard spaces;
    for (int n=0 ; n<SIZE ; n+=1) spaces[n] = board->blocks[n] ^ (uint16_t) -1;

    // header row
    struct coord pos = initpos;
    while (true) {
        pos = nextpos(spaces, pos);
        if (pos.x == -1) break;

        fshowpos(out, pos);
        fprintf(out, ",");
    }
    fprintf(out, "turn,score\n");

    // loop over all coordinates in spaces, output dwarf vs troll
    pos = initpos;
    while (true) {
        pos = nextpos(spaces, pos);
        if (pos.x == -1) break;

        if (dwarfat(board, pos)) fprintf(out, "-1");
        else if (trollat(board, pos)) fprintf(out, "1");
        else fprintf(out, "0");

        fprintf(out, ",");
    }
    fprintf(out, "%d,%e\n", board->isdwarfturn?-1:1, 0.0);

    for (int ix=0 ; ix<moves.used ; ix+=1) {
        domoveupdatecapts(board, & moves.moves[ix]);

        // loop over all coordinates in spaces, output dwarf vs troll
        pos = initpos;
        while (true) {
            pos = nextpos(spaces, pos);
            if (pos.x == -1) break;

            if (dwarfat(board, pos)) fprintf(out, "-1");
            else if (trollat(board, pos)) fprintf(out, "1");
            else fprintf(out, "0");

            fprintf(out, ",");
        }
        fprintf(out, "%d,%e\n", board->isdwarfturn?-1:1, (double) finalscore * (ix+1) / moves.used);
    }
}
