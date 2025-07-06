#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "inode.h"
#include "rm.h"
#include "ext2_global.h"
#include "path.h"

void free_inode(FILE* fp, struct ext2_super_block* sb, uint32_t inode_num) {
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint32_t group = (inode_num - 1) / sb->s_inodes_per_group;
    uint32_t index = (inode_num - 1) % sb->s_inodes_per_group;
    uint64_t gdt_offset = (block_size == 1024) ? 2 * block_size : block_size;
    struct ext2_group_desc gd;
    fseek(fp, gdt_offset + group * sizeof(gd), SEEK_SET);
    fread(&gd, sizeof(gd), 1, fp);
    uint32_t inode_table_block = gd.bg_inode_table;
    uint64_t inode_offset = (uint64_t)inode_table_block * block_size + index * sb->s_inode_size;
    struct ext2_inode zero_inode = {0};
    fseek(fp, inode_offset, SEEK_SET);
    fwrite(&zero_inode, sizeof(zero_inode), 1, fp);
    uint8_t* inode_bitmap = malloc(block_size);
    fseek(fp, gd.bg_inode_bitmap * block_size, SEEK_SET);
    fread(inode_bitmap, 1, block_size, fp);
    inode_bitmap[index / 8] &= ~(1 << (index % 8));
    fseek(fp, gd.bg_inode_bitmap * block_size, SEEK_SET);
    fwrite(inode_bitmap, 1, block_size, fp);
    free(inode_bitmap);
    sb->s_free_inodes_count++;
    gd.bg_free_inodes_count++;
    fseek(fp, BASE_OFFSET, SEEK_SET);
    fwrite(sb, sizeof(struct ext2_super_block), 1, fp);
    fseek(fp, gdt_offset + group * sizeof(gd), SEEK_SET);
    fwrite(&gd, sizeof(gd), 1, fp);
}

int is_dir_empty(FILE* fp, struct ext2_inode* dir_inode, uint32_t block_size) {
    uint8_t* block = malloc(block_size);
    for (int i = 0; i < 12 && dir_inode->i_block[i]; i++) {
        fseek(fp, dir_inode->i_block[i] * block_size, SEEK_SET);
        fread(block, 1, block_size, fp);
        uint32_t offset = 0;
        while (offset < block_size) {
            struct ext2_dir_entry* entry = (struct ext2_dir_entry*)(block + offset);
            if (entry->inode != 0 && !(entry->name_len == 1 && entry->name[0] == '.') && !(entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.')) {
                free(block);
                return 0;
            }
            offset += entry->rec_len;
        }
    }
    free(block);
    return 1;
}

void remove_entry(FILE* fp, struct ext2_super_block* sb, const char* name, int require_empty) {
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint8_t* block = malloc(block_size);
    for (int i = 0; i < 12 && current_inode->i_block[i]; i++) {
        fseek(fp, current_inode->i_block[i] * block_size, SEEK_SET);
        fread(block, 1, block_size, fp);
        uint32_t offset = 0;
        struct ext2_dir_entry* prev = NULL;
        while (offset < block_size) {
            struct ext2_dir_entry* entry = (struct ext2_dir_entry*)(block + offset);
            char entry_name[256] = {0};
            memcpy(entry_name, entry->name, entry->name_len);
            entry_name[entry->name_len] = '\0';
            if (strcmp(entry_name, name) == 0) {
                struct ext2_inode* target = get_inode(fp, sb, entry->inode);
                if ((target->i_mode & 0xF000) == 0x4000) {
                    if (require_empty && !is_dir_empty(fp, target, block_size)) {
                        printf("directory '%s' is not empty.\n", name);
                        free(target);
                        free(block);
                        return;
                    }
                }
                free_inode(fp, sb, entry->inode);
                if (prev) {
                    prev->rec_len += entry->rec_len;
                } else {
                    entry->inode = 0;
                }
                fseek(fp, current_inode->i_block[i] * block_size, SEEK_SET);
                fwrite(block, 1, block_size, fp);
                fflush(fp);
                printf("file '%s' removed with success.\n", name);
                free(target);
                free(block);
                return;
            }
            if (entry->rec_len == 0) break;
            prev = entry;
            offset += entry->rec_len;
        }
    }
    printf("file '%s' not found.\n", name);
    free(block);
}
