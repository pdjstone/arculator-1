#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

typedef size_t epos_t;

struct EMAP_S
{ // Map Indexing Struct
    uint32_t hash;
    uint32_t pos;
    uint32_t size;
};
typedef struct EMAP_S EMAP;

struct EFILE_S
{ // Virtual File Stream
    char *pos;
    char *end;
    size_t size;
    int err;
};
typedef struct EFILE_S EFILE;

static inline uint32_t hash(char *key)
{ // Hash Function: MurmurOAAT64
    uint32_t h = 3323198485ul;
    for (; *key; ++key)
    {
        h ^= *key;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}
