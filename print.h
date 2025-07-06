#ifndef EXT2_PRINT_H
#define EXT2_PRINT_H

#include "structs.h"
#include <stdio.h>
#include <stdint.h>

void format_permissions(uint16_t mode, char* out);
void format_time(uint32_t epoch, char* out, size_t out_size);
void info(FILE* fp, struct ext2_super_block* sb);
void list_directory(FILE *fp, struct ext2_super_block* sb, struct ext2_inode* inode);
void print_superblock(struct ext2_super_block* sb);
void print_groups(FILE* fp, struct ext2_super_block* sb);
void print_inode(FILE* fp, struct ext2_super_block* sb, int inode_num);
void attr_file(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_inode, uint32_t block_size);

#endif