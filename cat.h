#ifndef EXT2_CAT_H
#define EXT2_CAT_H

#include "structs.h"
#include <stdio.h>
#include <stdint.h>

void read_and_print_block(FILE* fp, uint8_t* buffer, uint32_t block_num, uint32_t bytes_to_read, uint32_t block_size);
void cat_file(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_inode, uint32_t block_size);

#endif // EXT2_CAT_H