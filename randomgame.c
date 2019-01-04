#include <stdio.h>

#include "thudlib.h"

const int NTRIES = 1000;

struct thudboard board_data;

int main(int numargs, char * args[])
{
    srand(time(NULL));

    struct thudboard * board = & board_data;
    struct move move;

    double scores[NTRIES];
    for (int tries=0 ; tries<NTRIES ; tries+=1) {
        setup(board);

        int moves = 0;
        int moves_since_capt = 0;
        while (moves_since_capt <= 20) //DRAW_DEADLINE)
        {
            struct movelist list = allmoves(board);
            if (list.used == 0) break;
            move = list.moves[rand() % list.used];
            domoveupdatecapts(board, & move);
            closelist(& list);

            moves += 1;
            if (move.numcapts) moves_since_capt = 0;
            else moves_since_capt += 1;
        }

        scores[tries] = score_game(board) / 1000.0;
    }

    double totalscore = 0;
    for (int ix=0 ; ix<NTRIES ; ix+=1) totalscore += scores[ix];
    double mean = totalscore / NTRIES;

    double sqerrs[NTRIES];
    for (int ix=0 ; ix<NTRIES ; ix+=1) {
        double err = scores[ix] - mean;
        sqerrs[ix] = err * err;
    }
    double totalsqerr = 0;
    for (int ix=0 ; ix<NTRIES ; ix+=1) totalsqerr += sqerrs[ix];
    double meansqerr = totalsqerr / NTRIES;

    printf("score mean = %g\n", mean);
    printf("score mean squared error = %g\n", meansqerr);
}
