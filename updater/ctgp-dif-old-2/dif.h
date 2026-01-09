#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "dif_archive.h"

u32 bswap32(u32 v);
int OpenExpFiles(const char *pack, const char *rmc, u8 *data, u32 size);
void ProcessDif(u8 *data, u32 size, FILE *file);
int dif_main(int argc, char **argv);