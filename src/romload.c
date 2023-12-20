/*Arculator 2.2 by Sarah Walker
  ROM loader*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arc.h"
#include "config.h"
#include "mem.h"
#include "embed.h"

/* re-enable this code when we can support it in the front-end */
#if 0
int loadertictac()
{
	int c,d;
	char s[10];
	FILE *f[4];
	int addr=0;
	uint8_t *romb = (uint8_t *)rom;
	for (c=0;c<16;c+=4)
	{
		for (d=0;d<4;d++)
		{
			sprintf(s,"%02i",(c|d)+1);
//                        rpclog("Opening %s\n",s);
			f[d]=embed_fopen(s,"rb");
			if (!f[d])
			{
//                                rpclog("File missing!\n");
				return -1;
			}
		}
		for (d=0;d<0x40000;d+=4)
		{
			romb[d+addr]=getc(f[0]);
			romb[d+addr+1]=getc(f[1]);
			romb[d+addr+2]=getc(f[2]);
			romb[d+addr+3]=getc(f[3]);
		}
		for (d=0;d<4;d++) fclose(f[d]);
		addr+=0x40000;
	}
	ignore_result(chdir(olddir));
	return 0;
}

int loadpoizone()
{
	int c,d;
	char s[10];
	FILE *f[4];
	int addr=0;
	uint8_t *romb = (uint8_t *)rom;
	return -1;
	for (c=0;c<24;c+=4)
	{
		if (c==12 || c==16)
		{
			addr+=0x40000;
			continue;
		}
		for (d=0;d<4;d++)
		{
			sprintf(s,"p_son%02i.bin",(c|d)+1);
//                        rpclog("Opening %s\n",s);
			f[d]=embed_fopen(s,"rb");
			if (!f[d])
			{
//                                rpclog("File missing!\n");
				return -1;
			}
		}
		for (d=0;d<0x40000;d+=4)
		{
			romb[d+addr]=getc(f[0]);
			romb[d+addr+1]=getc(f[1]);
			romb[d+addr+2]=getc(f[2]);
			romb[d+addr+3]=getc(f[3]);
		}
		for (d=0;d<4;d++) fclose(f[d]);
		addr+=0x40000;
	}
	ignore_result(chdir(olddir));
	return 0;
}
#endif

static int romAllocated=0;
int loadrom()
{
    char name[256];    
    sprintf(name, "roms/%s.rom", config_get_romset_name(romset));

    if (romAllocated)
    {
        free(rom);
        romAllocated = 0;
        mem_setrom(NULL);
    }

    char* embeddedRom = embed_data(name, NULL);
    if (embeddedRom) {
        rpclog("Found ROM embedded: %s\n", name);
        mem_setrom((uint32_t*) embeddedRom);
        return 0;
    }

    FILE *f = fopen(name, "rb");
    if (!f) {
        perror(name);
        return -1;
    }

    mem_setrom(malloc(0x200000));
    if (!rom)
        abort();

    rpclog("Loading ROM: %s\n", name);

    if (fread(rom, 0x200000, 1, f) != 1) {
        free(rom);
        mem_setrom(NULL);
        perror(name);
        return -1;
    }

	return 0;
}

int romset_available_mask = 0;

/*Establish which ROMs are available*/
int rom_establish_availability()
{
	for (int s = 0; s < ROM_MAX; s++)
	{
        char name[256];
        sprintf(name, "roms/%s.rom", config_get_romset_name(romset));
        if (embed_data(name, NULL) != NULL)
            romset_available_mask |= (1 << romset);
	}

	return (!romset_available_mask);
}

void rom_load_5th_column(void)
{
	FILE *f;
	char fn[512];

	rpclog("rom_load_5th_column: machine_type=%i\n", machine_type);
    if (rom_5th_column == NULL) {
        rom_5th_column = malloc(0x20000);
    }
	memset(rom_5th_column, 0xff, 0x20000);

	strcpy(fn, _5th_column_fn);

	/*If machine is an A4 and no 5th column ROM is specified, load in the default*/
	if (machine_type == MACHINE_TYPE_A4 && _5th_column_fn[0] == 0)
		append_filename(fn, exname, "roms/A4 5th Column.rom", sizeof(fn));

	rpclog("  %s\n", fn);
    char* embeddedRom = embed_data("roms/A4 5th Column.rom", NULL);
    if (embeddedRom) {
        rpclog("Found 5th column ROM embedded: %s\n", fn);
        free(rom_5th_column);
        rom_5th_column = (uint8_t*)embeddedRom;
        return;
    }

	f = embed_fopen(fn, "rb");
	if (f)
	{
		if (fread(rom_5th_column, 0x20000, 1, f) != 1) {
            perror(fn);
        }
		fclose(f);
	}
	else
		rpclog("rom_load_5th_column: file not found\n");
}

void rom_load_arc_support_extrom(void)
{
	char fn[512];
	FILE *f;

    char *embeddedRom = embed_data("roms/arcrom_ext", NULL);
    if (embeddedRom) {
        rpclog("Found ARC support ROM embedded: %s\n", fn);
        rom_arcrom = (uint8_t *)embeddedRom;
        return;
    }

	append_filename(fn, exname, "roms/arcrom_ext", 511);
	f = embed_fopen(fn, "rb");
	if (!f) {
		perror("roms/arcrom_ext");
		return;
	}
	ignore_result(fread(rom_arcrom, 0x10000, 1, f));
	fclose(f);
}
