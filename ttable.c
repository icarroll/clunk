#include "thudlib.h"
#include "ttable.h"

#include <stdio.h>
#include <stdlib.h>
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

int TTABLESIZE;
struct tableentry * ttable;

int ttindex(hash_t hash)
{
    return hash % TTABLESIZE;
}

void initttable(int memuse)
{
    TTABLESIZE = memuse / sizeof(struct tableentry);

    // allocate memory for table
    void * mem = mmap(NULL, memuse, PROT_READ | PROT_WRITE,
                      MAP_ANON | MAP_PRIVATE, -1, 0);
    ttable = (struct tableentry *) mem;

    // read in table data if present
    FILE * book = fopen(BOOKFILENAME, "r");
    if (book)
    {
        printf("Reading opening book...");
        fflush(stdout);
        struct tableentry temp;
        while (fread(& temp, sizeof(struct tableentry), 1, book))
        {
            int index = ttindex(temp.hash);
            ttable[index] = temp;
        }
        printf(" done.\n");
        fflush(stdout);
    }
}

void ttput(struct tableentry newentry)
{
    int index = ttindex(newentry.hash);
    struct tableentry * entry = & ttable[index];

    * entry = newentry;
}

struct tableentry * ttget(hash_t hash)
{
    int index = ttindex(hash);

    if (ttable[index].hash == hash) return & ttable[index];
    else return NULL;
}