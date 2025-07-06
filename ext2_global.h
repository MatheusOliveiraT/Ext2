#ifndef EXT2_GLOBAL_H
#define EXT2_GLOBAL_H

#include <stdint.h>
#include <stdbool.h> 
#include "structs.h"

extern struct ext2_inode* current_inode;
extern uint32_t current_inode_number;
extern char current_path[256];

#endif // EXT2_GLOBAL_H