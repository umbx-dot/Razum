#include "common/arena.h"

#include <stdlib.h>

#include "common/diagnostics.h"

enum { ARENA_CHUNK_SIZE = 64 * 1024, ARENA_ALIGNMENT = 16 };

static unsigned char *chunk;
static size_t chunk_used;
static size_t chunk_size;

void *arena_alloc(size_t size) {
    size_t aligned = (size + ARENA_ALIGNMENT - 1) & ~(size_t)(ARENA_ALIGNMENT - 1);
    if (chunk == NULL || chunk_used + aligned > chunk_size) {
        chunk_size = aligned > ARENA_CHUNK_SIZE ? aligned : ARENA_CHUNK_SIZE;
        chunk = calloc(1, chunk_size);
        if (chunk == NULL) {
            fatal_error("out of memory");
        }
        chunk_used = 0;
    }
    void *result = chunk + chunk_used;
    chunk_used += aligned;
    return result;
}
