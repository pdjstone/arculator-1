/* Based on https://github.com/weigert/c-embed/ which reimplements FILE as EFILE,
 * eopen for fopen etc.
 * 
 * We're looking for something more invasive, so we use embed_fopen as a hook for fopen
 * and embed_data to get a direct pointer to embedded data, where we have to make
 * changes to the Arculator code base.
 * 
 * We rely on fmemopen to create a FILE* from a pointer to memory, for which we need
 * to supply a small Windows implementation.
 */

#include "arc.h"
#include "embed.h"

extern char cembed_map_start; // Embedded Indexing Structure
extern uint64_t cembed_map_size;

extern char cembed_fs_start;  // Embedded Virtual File System
extern uint64_t cembed_fs_size;

static EFILE* eopen(const char* file, const char* mode){

  EMAP* map = (EMAP*)(&cembed_map_start);
  const char* end = ((char*)&cembed_map_start) + cembed_map_size;

  if( map == NULL || end == NULL )
    return NULL;

  const uint32_t key = hash((char*)file);
  while( ((char*)map != end) && (map->hash != key) ) {
    map++;
  }

  if(map->hash != key)
    return NULL;

  EFILE* e = (EFILE*)malloc(sizeof *e);
  e->pos = (&cembed_fs_start + map->pos);
  e->end = (&cembed_fs_start + map->pos + map->size);
  e->size = map->size;

  return e;

}

char *embed_data(const char *file, size_t *size)
{
    char *data;
    EFILE *e = eopen(file, "rb");
    if (e == NULL)
        return NULL;
    data = e->pos;
    if (size)
        *size = e->size;
    free(e);
    return data;
}

#ifdef _WIN32
/* thank you https://github.com/Arryboom/fmemopen_windows  */
#include <windows.h>
#include <share.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

FILE *fmemopen(void *buf, size_t len, const char *type)
{
    int fd;
    FILE *fp;
    char tp[MAX_PATH - 13];
    char fn[MAX_PATH + 1];
    int *pfd = &fd;
    int retner = -1;
    char tfname[] = "MemTF_";
    if (!GetTempPathA(sizeof(tp), tp))
        return NULL;
    if (!GetTempFileNameA(tp, tfname, 0, fn))
        return NULL;
    retner = _sopen_s(pfd, fn, _O_CREAT | _O_SHORT_LIVED | _O_TEMPORARY | _O_RDWR | _O_BINARY | _O_NOINHERIT, _SH_DENYRW, _S_IREAD | _S_IWRITE);
    if (retner != 0)
        return NULL;
    if (fd == -1)
        return NULL;
    fp = _fdopen(fd, "wb+");
    if (!fp)
    {
        _close(fd);
        return NULL;
    }
    /*File descriptors passed into _fdopen are owned by the returned FILE * stream.If _fdopen is successful, do not call _close on the file descriptor.Calling fclose on the returned FILE * also closes the file descriptor.*/
    fwrite(buf, len, 1, fp);
    rewind(fp);
    return fp;
}
#endif

FILE* embed_fopen(const char *file, const char *mode)
{
    size_t size;
    if (!file || file[0] == 0) {
        return NULL;
    }
    if (strcmp(mode, "rb") != 0 && strcmp(mode, "r") != 0)
    {
        rpclog("embed_fopen(%s, %s) - fallthrough\n", file, mode);
        return real_fopen(file, mode);
    }
    char *p = embed_data(file, &size);
    if (p == NULL)
    {
        rpclog("embed_fopen(%s, %s) - fallthrough\n", file, mode);
        return real_fopen(file, mode);
    }
    rpclog("embed_fopen(%s, %s) - embedded\n", file, mode);
    return fmemopen(p, size, mode);
}
