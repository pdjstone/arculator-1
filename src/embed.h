#ifndef _EMBED_H
#define _EMBED_H

#include <stdio.h>

#include "c-embed.h"

/* Retrieve pointer to embedded file given by `file`, optionally filling in size in `size` 
 * 
 * Returns NULL if the file doesn't exist.
 */
char *embed_data(const char *file, size_t *size);
static inline size_t embed_data_size(const char *file) {
    size_t size;
    if (embed_data(file, &size))
       return size;
    return -1;
}

/* Open the embedded file given by `file`. If the file doesn't exist, or `mode` specifies
 * writing, will fall through to ordinary `fopen()`.
 * 
 * This will be a useful stop-gap to clean up inconsistent file access patterns across the
 * codebase and allow us to hook
 */
FILE *embed_fopen(const char *file, const char *mode);

static inline FILE *real_fopen(const char *file, const char *mode)
{
    return fopen(file, mode);
}

#define fopen(f, m) embed_fopen(f, m)

#endif
