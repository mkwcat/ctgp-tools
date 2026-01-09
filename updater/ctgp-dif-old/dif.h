#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "dif_archive.h"

#define MAXFILES 15

static u8 *dia_data;
static int dia_size;

static FILE* outfile;

static u8* expdata[MAXFILES];
static u32 expfilesize[MAXFILES];
static u32 expcursors[MAXFILES + 1];

u32 bswap32(u32 v);
int OpenExpFiles(const char *pack, const char *rmc, u8 *data, u32 size);
void ProcessDif(u8 *data, u32 size, FILE *file);
int dif_main(int argc, char **argv);