#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>

#include "arc.h"

#include <emscripten.h>

/**
 * These functions allow HostFS to work correctly on top of Emscripten's MemFS which
 * assumes that file paths are UTF8 encoded strings. The Emscripten conversion between
 * (assumed) UTF-8 file paths and Javascript UTF-16 strings causes file paths encoded 
 * in other character sets to be mangled. This is a problem for RISC OS file paths 
 * that have characters outside of the 7-bit ASCII range (e.g. !élite).
 * 
 * The RISC OS character encoding is a variant of ISO-8859-1. However, instead of doing 
 * a proper conversion, the hostfs_ros_to_utf8 and hostfs_utf8_to_ros functions below
 * cheat by using String.fromCharCode / charCodeAt to map RISC OS strings directly to
 * UTF-16. This isn't perfect, but it round-trips from RISC OS to HostFS/MemFS without
 * getting mangled.
 * 
 * Arculator running natively doesn't have this problem (on Linux, at least) because
 * there, the filesystem just treats paths as a sequence of bytes - you get out what 
 * you put in. Although this doesn't cause HostFS to break, it does mean that HostFS
 * filenames can look mangled when viewed natively.
 */ 

EM_JS(void, hostfs_ros_to_utf8, (const char *unicode_str_ptr, const char *ros_str_ptr), {
    let str = "";
    for (let i=ros_str_ptr; HEAPU8[i]!=0; i++) {
        str += String.fromCharCode(HEAPU8[i]);
    }
    let utf8_bytes = new TextEncoder().encode(str);
    let i = 0;
    for (;i<utf8_bytes.byteLength;i++) {
        HEAPU8[unicode_str_ptr+i] = utf8_bytes[i];
    }
    HEAPU8[unicode_str_ptr+i] = 0;
});

EM_JS(void, hostfs_utf8_to_ros, (const char *ros_pathname, const char *unicode_pathname), {
  let str = UTF8ToString(unicode_pathname);
  let i=0;
  for (;i<str.length; i++) {
    HEAPU8[ros_pathname+i] = str.charCodeAt(i);
  }
  HEAPU8[ros_pathname+i] = 0;
});

FILE *fopen_hostfs_emscripten(const char *pathname, const char *mode) {
    char unicode_path[PATH_MAX];
    hostfs_ros_to_utf8(unicode_path, pathname);
    //rpclog("fopen_hostfs_emscripten %s %s\n", unicode_path, mode);
    return fopen(unicode_path, mode);
}

int rename_hostfs_emscripten(const char *oldpath, const char *newpath) {
    char unicode_oldpath[PATH_MAX];
    char unicode_newpath[PATH_MAX];

    hostfs_ros_to_utf8(unicode_oldpath, oldpath);
    hostfs_ros_to_utf8(unicode_newpath, newpath);
    
    //rpclog("rename_hostfs_emscripten %s -> %s\n", unicode_oldpath, unicode_newpath);
    return rename(unicode_oldpath, unicode_newpath);
}


DIR *opendir_hostfs_emscripten(const char *name) {
    char unicode_name[PATH_MAX];
    hostfs_ros_to_utf8(unicode_name, name);
    //rpclog("opendir_hostfs_emscripten %s\n", unicode_name);
    return opendir(unicode_name);
}

struct dirent *readdir_hostfs_emscripten(DIR *dirp) {
    struct dirent *dir = readdir(dirp);
    if (dir != NULL) {
        //rpclog("readdir_hostfs_emscripten -> %s\n", dir->d_name);
        hostfs_utf8_to_ros(dir->d_name, dir->d_name);
    } else {
        //rpclog("readdir_hostfs_emscripten -> END\n");
    }
    return dir;
}

int stat_hostfs_emscripten(const char *pathname, struct stat *statbuf) {
    char unicode_pathname[PATH_MAX];
    hostfs_ros_to_utf8(unicode_pathname, pathname);
    //rpclog("stat_hostfs_emscripten %s\n", unicode_pathname);
    return stat(unicode_pathname, statbuf);
}

int utime_hostfs_emscripten(const char *filename, const struct utimbuf *times) {
    char unicode_filename[PATH_MAX];
    hostfs_ros_to_utf8(unicode_filename, filename);
    return utime(unicode_filename, times);
}

int mkdir_hostfs_emscripten(const char *pathname, mode_t mode) {
    char unicode_pathname[PATH_MAX];
    hostfs_ros_to_utf8(unicode_pathname, pathname);
    //rpclog("mkdir_hostfs_emscripten %s\n", unicode_pathname);
    return mkdir(unicode_pathname, mode);
}

