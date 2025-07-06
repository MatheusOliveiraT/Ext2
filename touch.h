#ifndef EXT2_TOUCH_H
#define EXT2_TOUCH_H

#include "structs.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BASE_OFFSET 1024

int find_free_inode(FILE* fp, struct ext2_super_block* sb, struct ext2_group_desc* gd, uint32_t group, uint32_t block_size, uint32_t inodes_per_group);
void add_directory_entry(FILE* fp, struct ext2_inode* dir_inode, uint32_t inode_num, const char* name, uint32_t block_size, bool is_directory);
void touch(FILE* fp, struct ext2_super_block* sb, struct ext2_inode* current_inode, const char* filename, uint32_t block_size);
int find_free_block(FILE* fp, struct ext2_super_block* sb, struct ext2_group_desc* gd, uint32_t group, uint32_t block_size, uint32_t blocks_per_group);
void mkdir_ext2(FILE* fp, struct ext2_super_block* sb, struct ext2_inode* current_inode, const char* dirname, uint32_t* current_inode_number, uint32_t block_size);
void cp_file(FILE* fp, struct ext2_super_block* sb, struct ext2_inode* current_inode, const char* src, const char* dst, uint32_t block_size);
void rename_entry(FILE* fp, struct ext2_inode* dir_inode, const char* old_name, const char* new_name, uint32_t block_size);

#endif