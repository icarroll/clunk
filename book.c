#include "ttable.h"
#include "thudlib.h"

#include <stdio.h>
#include <stdlib.h>

FILE * book;

int TTABLESIZE;
struct tableentry * ttable;

struct thudboard board_data;

int main(int numargs, char * args[])
{
    struct thudboard * board = & board_data;

    int memuse = 1024 * 1024 * (numargs > 1 ? strtol(args[1], NULL, 10)
                                            : sizeof(struct tableentry));

    setupgame(board, memuse);

    search(board, 4);

    book = fopen(BOOKFILENAME, "w");
    for (int i=0; i < TTABLESIZE; ++i)
    {
        if (ttable[i].hash != 0)
        {
            fwrite(& (ttable[i]), sizeof(struct tableentry), 1, book);
        }
    }
    fclose(book);
}

// -----

#include "ttable.h"

#include <sys/mman.h>

hash_t turnhash;
hash_t dwarfhash[SIZE*SIZE];
hash_t trollhash[SIZE*SIZE];

hash_t randomhash(void)
{
    return (hash_t) random() << 32 ^ random();
}

void inithash(void)
{
    turnhash = randomhash();

    for (int i=0; i < SIZE*SIZE; ++i)
    {
        dwarfhash[i] = randomhash();
        trollhash[i] = randomhash();
    }
}

void hashturn(struct thudboard * board)
{
    board->hash ^= turnhash;
}

void hashdwarf(struct thudboard * board, struct coord pos)
{
    board->hash ^= dwarfhash[pos.y*SIZE + pos.x];
}

void hashtroll(struct thudboard * board, struct coord pos)
{
    board->hash ^= trollhash[pos.y*SIZE + pos.x];
}

void initttable(int memuse)
{
    TTABLESIZE = memuse / sizeof(struct tableentry);

    void * mem = mmap(NULL, memuse, PROT_READ | PROT_WRITE,
                      MAP_ANON | MAP_PRIVATE, -1, 0);
    ttable = (struct tableentry *) mem;
}

int ttindex(hash_t hash)
{
    return hash % TTABLESIZE;
}

void ttput(struct thudboard * board, int depth, int trmin, int dwmax)
{
    int index = ttindex(board->hash);

    if (ttable[index].depth > depth) return;

    struct tableentry temp =
    {
        board->hash,
        depth,
        trmin,
        dwmax,
    };
    ttable[index] = temp;
}

struct tableentry * ttget(hash_t hash)
{
    int index = ttindex(hash);

    if (ttable[index].hash == hash) return & (ttable[index]);
    else return NULL;
}
