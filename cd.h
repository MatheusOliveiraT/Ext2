#ifndef EXT2_CD_H
#define EXT2_CD_H

#include "structs.h"
#include <stdio.h>
#include <stdint.h>

void update_path(const char* path);
void change_directory(FILE* fp, struct ext2_super_block* sb, const char* path, uint32_t block_size);

#endif