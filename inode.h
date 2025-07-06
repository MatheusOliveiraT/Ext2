#ifndef EXT2_INODE_H
#define EXT2_INODE_H

#include "structs.h"
#include <stdio.h>

struct ext2_inode* get_inode(FILE* fp, struct ext2_super_block* sb, uint32_t inode_num);

#endif // EXT2_INODE_H