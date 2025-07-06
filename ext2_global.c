#include "ext2_global.h"
#include <stdlib.h>

struct ext2_inode* current_inode = NULL;
uint32_t current_inode_number = 2;
char current_path[256] = "/";