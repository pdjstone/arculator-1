#include <stdio.h>

void adf_init();

void adf_load(int drive, FILE *f, int writeprot);
void adl_load(int drive, FILE *f, int writeprot);
void adf_arcdd_load(int drive, FILE *f, int writeprot);
void adf_archd_load(int drive, FILE *f, int writeprot);
void adf_loadex(int drive, FILE *fn, int writeprot, int sectors, int size, int dblside, int dblstep, int density, int sector_offset);

void ssd_load(int drive, FILE *f, int writeprot);
void dsd_load(int drive, FILE *f, int writeprot);
