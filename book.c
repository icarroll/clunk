#include "ttable.h"
#include "thudlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

enum {DEPTH = 5};

FILE * book;

int TTABLESIZE;
struct tableentry * ttable;

void initbtable(int memuse);
int BTABLESIZE;
struct tableentry * btable;

struct thudboard board_data;

int main(int numargs, char * args[])
{
    struct thudboard * board = & board_data;

    int memuse = 1024 * 1024 * (numargs > 1 ? strtol(args[1], NULL, 10)
                                            : sizeof(struct tableentry));

    setupgame(board, memuse);

    initbtable(1024 * 1024 * sizeof(struct tableentry));

    for (int depth=2; depth <= DEPTH; depth += 1)
    {
        absearch(board, depth, INT_MAX, INT_MIN, INT_MAX, NULL, 0, NULL);
    }

    book = fopen(BOOKFILENAME, "w");
    for (int i=0; i < BTABLESIZE; ++i)
    {
        if (btable[i].hash != 0)
        {
            fwrite(& (btable[i]), sizeof(struct tableentry), 1, book);
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

void initbtable(int memuse)
{
    BTABLESIZE = memuse / sizeof(struct tableentry);

    void * mem = mmap(NULL, memuse, PROT_READ | PROT_WRITE,
                      MAP_ANON | MAP_PRIVATE, -1, 0);
    btable = (struct tableentry *) mem;
}

int ttindex(hash_t hash)
{
    return hash % TTABLESIZE;
}

int btindex(hash_t hash)
{
    return hash % BTABLESIZE;
}

void ttput(struct tableentry entry)
{
    int index = ttindex(entry.hash);
    ttable[index] = entry;

    int bindex = btindex(entry.hash);
    if (entry.depth > btable[bindex].depth) btable[bindex] = entry;
}

struct tableentry * ttget(hash_t hash)
{
    int index = ttindex(hash);

    if (ttable[index].hash == hash) return & (ttable[index]);
    else return NULL;
}
