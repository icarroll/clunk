#include <stdio.h>

#include "thudlib.h"

enum {SEARCHSECS = 1};
enum {SEARCHDEPTH = 4};

struct thudboard board_data;

int main(int numargs, char * args[]) {
    struct movelist moves;
    initlist(& moves);

    struct thudboard * board = & board_data;
    struct move move;

    long memuse = 4l * 1024 * 1024 * 1024;
    setupgame(board, memuse);
    LOGF = fopen("/dev/null", "w");

    int starttime = time(NULL);
    srandom(starttime);

    int moves_since_capt = 0;
    while (moves_since_capt <= DRAW_DEADLINE)
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
    sprintf(filename, "run-depth%d-%d", SEARCHDEPTH, starttime);
    FILE * out = fopen(filename, "w");
    for (int ix=0 ; ix<moves.used ; ix+=1) {
        //for (int x
    }
}
