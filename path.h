#ifndef EXT2_PATH_H
#define EXT2_PATH_H

#include "structs.h" 
#include <stdio.h>
#include <stdbool.h>

struct ext2_inode* resolve_path(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* start_inode, uint32_t start_inode_number, uint32_t block_size, uint32_t* final_inode_number, bool update_global_context);

#endif // EXT2_PATH_H