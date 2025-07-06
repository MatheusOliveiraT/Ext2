#ifndef EXT2_PRINT_H
#define EXT2_PRINT_H

#include "structs.h"
#include "inode.h"
#include "path.h"
#include "ext2_global.h"
#include "print.h"
#include <stdio.h>
#include <stdint.h>

void format_permissions(uint16_t mode, char* out);
void format_time(uint32_t epoch, char* out, size_t out_size);
void info(FILE* fp, struct ext2_super_block* sb);
void list_directory(FILE *fp, struct ext2_super_block* sb, struct ext2_inode* inode);
void print_superblock(struct ext2_super_block* sb);
void print_groups(FILE* fp, struct ext2_super_block* sb);
void print_inode(FILE* fp, struct ext2_super_block* sb, uint32_t inode_num);
void print_rootdir(FILE* fp, struct ext2_super_block* sb, uint32_t block_size);
void print_dir(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_dir_inode, uint32_t current_dir_inode_num, uint32_t block_size);
int get_bit(FILE* fp, uint32_t bitmap_block, uint32_t bit_index, uint32_t block_size);
int read_block(FILE* fp, uint32_t block_num, uint32_t block_size, uint8_t* buffer);
struct ext2_group_desc* get_group_descriptor(FILE* fp, struct ext2_super_block* sb, uint32_t group_id);
void print_inode_bitmap(FILE* fp, struct ext2_super_block* sb, uint32_t group_id, uint32_t block_size);
void print_block_bitmap(FILE* fp, struct ext2_super_block* sb, uint32_t group_id, uint32_t block_size);
void print_block_content(FILE* fp, uint32_t block_num, uint32_t block_size);
void attr_file(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_inode, uint32_t block_size);

#endif