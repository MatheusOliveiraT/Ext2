#ifndef EXT2_RM_H
#define EXT2_RM_H

#include "structs.h"
#include <stdio.h>
#include <stdint.h>

#define BASE_OFFSET 1024

void free_inode(FILE* fp, struct ext2_super_block* sb, uint32_t inode_num);
int is_dir_empty(FILE* fp, struct ext2_inode* dir_inode, uint32_t block_size);
void remove_entry(FILE* fp, struct ext2_super_block* sb, const char* name, int require_empty);

#endif