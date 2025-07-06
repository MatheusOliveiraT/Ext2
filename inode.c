#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "structs.h"
#include "inode.h"

struct ext2_inode* get_inode(FILE* fp, struct ext2_super_block* sb, uint32_t inode_num) {
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint32_t inodes_per_group = sb->s_inodes_per_group;
    uint32_t group = (inode_num - 1) / inodes_per_group;
    uint32_t index = (inode_num - 1) % inodes_per_group;
    uint64_t gdt_offset = 2 * block_size;
    struct ext2_group_desc gd;
    fseek(fp, gdt_offset + group * sizeof(gd), SEEK_SET);
    fread(&gd, sizeof(gd), 1, fp);
    uint32_t inode_table_block = gd.bg_inode_table;
    uint32_t inode_size = sb->s_inode_size;
    uint64_t inode_offset = ((uint64_t)inode_table_block * block_size) + (index * inode_size);
    struct ext2_inode* inode = malloc(sizeof(struct ext2_inode));
    if (!inode) {
        perror("malloc");
        return NULL;
    }
    fseek(fp, inode_offset, SEEK_SET);
    fread(inode, inode_size, 1, fp);
    return inode;
}