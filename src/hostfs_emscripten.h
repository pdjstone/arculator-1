#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>

#include <emscripten.h>

#ifndef __HOSTFS_EMSCRIPTEN__
#define __HOSTFS_EMSCRIPTEN__
FILE *fopen_hostfs_emscripten(const char *pathname, const char *mode);
int rename_hostfs_emscripten(const char *oldpath, const char *newpath);
DIR *opendir_hostfs_emscripten(const char *name);
struct dirent *readdir_hostfs_emscripten(DIR *dirp);
int stat_hostfs_emscripten(const char *pathname, struct stat *statbuf);
int utime_hostfs_emscripten(const char *filename, const struct utimbuf *times);
int mkdir_hostfs_emscripten(const char *pathname, mode_t mode);

// Previously overriden by embed.h which is included by build process
#undef fopen

// Redefine file funtions used in hostfs.c and hostfs-unix.c 
// to use the conversion functions above. This is a bit hacky
// but lets us fix HostFS running on Emscripten MemFS with minimal
// changes to original code.
#define fopen fopen_hostfs_emscripten
#define rename rename_hostfs_emscripten
#define opendir opendir_hostfs_emscripten
#define readdir readdir_hostfs_emscripten
#define mkdir mkdir_hostfs_emscripten

#define utime utime_hostfs_emscripten
#define stat(path, info) stat_hostfs_emscripten(path, info)
#endif