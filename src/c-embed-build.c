/*
# c-embed
# embed virtual file systems into an c program
# - at build time
# - with zero dependencies
# - with zero code modifications
# - with zero clutter in your program
# author: nicholas mcdonald 2022, updated for Arculator Matthew Bloch 2023
*/

#define CEMBED_BUILD

#include "c-embed.h"
#include <dirent.h>
#include <string.h>

FILE* ms = NULL;    // Mapping Structure
FILE* fs = NULL;    // Virtual Filesystem
FILE* file = NULL;  // Embed Target File Pointer
FILE* out = NULL;
u_int32_t pos = 0;  // Current Position

void cembed(char* filename){
  char *base = filename+strlen(filename)-1;

  file = fopen(filename, "rb");  // Open the Embed Target File
  if(file == NULL){
    printf("Failed to open file %s.", filename);
    return;
  }

  fseek(file, 0, SEEK_END);     // Define Map
  EMAP map = {hash(filename), pos, (u_int32_t)ftell(file)};
  rewind (file);

  fprintf(stderr, "mapping %s and %s\n", filename, base);

  char* buf = malloc(map.size);
  if(buf == NULL){
    printf("Memory error for file %s.", filename);
    return;
  }

  u_int32_t result = fread(buf, 1, map.size, file);
  if(result != map.size){
    printf("Read error for file %s.", filename);
    return;
  }

  fwrite(&map, sizeof map, 1, ms);  // Write Mapping Structure
  fwrite(buf, map.size, 1, fs);     // Write Virtual Filesystem

  free(buf);        // Free Buffer
  fclose(file);     // Close the File
  file = NULL;      // Reset the Pointer
  pos += map.size;  // Shift the Index Position

  while ((pos & 63) != 0) { // Pad to 64-byte boundary
      fputc(0, fs);
      pos++;
  }
}

#define CEMBED_DIRENT_FILE 8
#define CEMBED_DIRENT_DIR 4
#define CEMBED_MAXPATH 512

void iterdir(char* d){

  char* fullpath = (char*)malloc(CEMBED_MAXPATH*sizeof(char));

  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir(d)) != NULL) {

    while ((ent = readdir(dir)) != NULL) {

      if(strcmp(ent->d_name, ".") == 0) continue;
      if(strcmp(ent->d_name, "..") == 0) continue;

      if(ent->d_type == CEMBED_DIRENT_FILE){
        strcpy(fullpath, d);
        strcat(fullpath, "/");
        strcat(fullpath, ent->d_name);
        cembed(fullpath);
      }

      else if(ent->d_type == CEMBED_DIRENT_DIR){
        strcpy(fullpath, d);
        strcat(fullpath, "/");
        strcat(fullpath, ent->d_name);
        iterdir(fullpath);
      }

    }

    closedir(dir);

  }

  else {

    strcpy(fullpath, d);
    cembed(fullpath);

  }

  free(fullpath);

}

void hexout(FILE* in, FILE* out, char* name){
  int c;

  fprintf(out, "const unsigned char %s_start[] = ", name);

  int col=0, nodigit=0;
  while((c = fgetc(in)) != EOF){
    if (col == 0) {
        fputs("\n \"", out);
    }

    if ((nodigit && c >= '0' && c <= '7') || c < 32 || c > 126 || c == '?' || c == '<' || c == '>' || c == ':' || c == '%')
    {
        col += fprintf(out, "\\%o", c);
        nodigit = 1;
    }
    else
    {
        if (c == '\\' || c == '"')
        {
            fputc('\\', out);
            col++;
        }
        fputc(c, out);
        col++;
        nodigit = 0;
    }
    if (col >= 77) {
        fputc('"', out);
        col = 0;
    }
  }
  if (col != 0) {
      fputs("\"", out);
  }
  fputs(";\n", out);
  fprintf(out, "const unsigned long long %s_size = %ld;\n", name, ftell(in));
  fprintf(out, "const unsigned char *%s_end = %s_start + %s_size;\n", name, name, name);
}

int main(int argc, char* argv[]){

  if(argc <= 1)
    return 0;

  // Build the Mapping Structure and Virtual File System

  ms = tmpfile();
  fs = tmpfile();

  if(ms == NULL || fs == NULL){
    printf("Failed to initialize map and filesystem. Check permissions.");
    return 0;
  }

  for(int i = 1; i < argc; i++)
    iterdir(argv[i]);

  rewind(ms);
  rewind(fs);

  hexout(ms, stdout, "cembed_map");
  hexout(fs, stdout, "cembed_fs");

  fclose(ms);
  fclose(fs);

  return 0;

}
