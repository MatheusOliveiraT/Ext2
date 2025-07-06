#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "structs.h"
#include "touch.h"

int find_free_inode(FILE* fp, struct ext2_super_block* sb, struct ext2_group_desc* gd, uint32_t group, uint32_t block_size, uint32_t inodes_per_group) {
    uint8_t* inode_bitmap = malloc(block_size);
    fseek(fp, gd->bg_inode_bitmap * block_size, SEEK_SET);
    fread(inode_bitmap, block_size, 1, fp);
    for (uint32_t i = 0; i < inodes_per_group; i++) {
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) {
            inode_bitmap[i / 8] |= (1 << (i % 8));
            fseek(fp, gd->bg_inode_bitmap * block_size, SEEK_SET);
            fwrite(inode_bitmap, block_size, 1, fp);
            sb->s_free_inodes_count--;
            gd->bg_free_inodes_count--;
            fseek(fp, BASE_OFFSET, SEEK_SET);
            fwrite(sb, sizeof(struct ext2_super_block), 1, fp);
            uint64_t gdt_offset = (block_size == 1024) ? 2 * block_size : block_size;
            fseek(fp, gdt_offset + group * sizeof(*gd), SEEK_SET);
            fwrite(gd, sizeof(*gd), 1, fp);
            free(inode_bitmap);
            return i + 1;
        }
    }
    free(inode_bitmap);
    return -1;
}

void add_directory_entry(FILE* fp, struct ext2_inode* dir_inode, uint32_t inode_num, const char* name, uint32_t block_size, bool is_directory) {
    uint8_t* block = malloc(block_size);
    for (int i = 0; i < 12; i++) {
        if (!dir_inode->i_block[i]) continue;
        fseek(fp, dir_inode->i_block[i] * block_size, SEEK_SET);
        fread(block, block_size, 1, fp);
        uint32_t offset = 0;
        while (offset < block_size) {
            struct ext2_dir_entry* entry = (struct ext2_dir_entry*)(block + offset);
            uint32_t actual_len = 8 + entry->name_len + ((4 - (entry->name_len % 4)) % 4);
            uint16_t prev_rec_len = entry->rec_len;
            if (prev_rec_len >= actual_len + 8 + strlen(name)) {
                entry->rec_len = actual_len;
                struct ext2_dir_entry* new_entry = (struct ext2_dir_entry*)((uint8_t*)entry + actual_len);
                new_entry->inode = inode_num;
                new_entry->rec_len = prev_rec_len - actual_len;
                new_entry->name_len = strlen(name);
                if (is_directory) {
                    new_entry->file_type = 2;
                } else {
                    new_entry->file_type = 1;
                }
                memcpy(new_entry->name, name, new_entry->name_len);
                uint8_t* name_end = (uint8_t*)new_entry->name + new_entry->name_len;
                while ((uintptr_t)(name_end - (uint8_t*)new_entry) % 4 != 0) {
                    *name_end = 0;
                    name_end++;
                }
                fseek(fp, dir_inode->i_block[i] * block_size, SEEK_SET);
                fwrite(block, block_size, 1, fp);
                fflush(fp);
                free(block);
                return;
            }
            if (entry->rec_len == 0) break;
            offset += entry->rec_len;
        }
    }
    free(block);
}

void touch(FILE* fp, struct ext2_super_block* sb, struct ext2_inode* current_inode, const char* filename, uint32_t block_size) {
    uint32_t group = 0;
    struct ext2_group_desc gd;
    uint64_t gdt_offset = (block_size == 1024) ? 2 * block_size : block_size;
    fseek(fp, gdt_offset + group * sizeof(gd), SEEK_SET);
    fread(&gd, sizeof(gd), 1, fp);
    int index = find_free_inode(fp, sb, &gd, 0, block_size, sb->s_inodes_per_group);
    if (index < 0) {
        printf("no inode available.\n");
        return;
    }
    uint32_t inode_num = group * sb->s_inodes_per_group + index;
    struct ext2_inode new_inode = {0};
    new_inode.i_mode = 0x81A4; // regular file, rw-r--r--
    new_inode.i_uid = 0;
    new_inode.i_gid = 0;
    new_inode.i_links_count = 1;
    new_inode.i_size = 0;
    new_inode.i_blocks = 0;
    new_inode.i_ctime = new_inode.i_mtime = new_inode.i_atime = time(NULL);
    uint64_t inode_offset = ((uint64_t)gd.bg_inode_table * block_size) + (index * sb->s_inode_size);
    fseek(fp, inode_offset, SEEK_SET);
    fwrite(&new_inode, sizeof(struct ext2_inode), 1, fp);
    add_directory_entry(fp, current_inode, inode_num, filename, block_size, false);
    printf("file '%s' created with success.\n", filename);
}

int find_free_block(FILE* fp, struct ext2_super_block* sb, struct ext2_group_desc* gd, uint32_t group, uint32_t block_size, uint32_t blocks_per_group) {
    uint8_t* block_bitmap = malloc(block_size);
    fseek(fp, gd->bg_block_bitmap * block_size, SEEK_SET);
    fread(block_bitmap, block_size, 1, fp);
    for (uint32_t i = 0; i < blocks_per_group; i++) {
        if (!(block_bitmap[i / 8] & (1 << (i % 8)))) {
            block_bitmap[i / 8] |= (1 << (i % 8));
            fseek(fp, gd->bg_block_bitmap * block_size, SEEK_SET);
            fwrite(block_bitmap, block_size, 1, fp);
            sb->s_free_blocks_count--;
            gd->bg_free_blocks_count--;
            fseek(fp, BASE_OFFSET, SEEK_SET);
            fwrite(sb, sizeof(struct ext2_super_block), 1, fp);
            uint64_t gdt_offset = (block_size == 1024) ? 2 * block_size : block_size;
            fseek(fp, gdt_offset + group * sizeof(*gd), SEEK_SET);
            fwrite(gd, sizeof(*gd), 1, fp);
            free(block_bitmap);
            return i + gd->bg_block_bitmap + 1 - gd->bg_block_bitmap;
        }
    }
    free(block_bitmap);
    return -1;
}

void mkdir_ext2(FILE* fp, struct ext2_super_block* sb, struct ext2_inode* current_inode, const char* dirname, uint32_t* current_inode_number, uint32_t block_size) {
    uint32_t group = 0;
    struct ext2_group_desc gd;
    uint64_t gdt_offset = (block_size == 1024) ? 2 * block_size : block_size;
    fseek(fp, gdt_offset + group * sizeof(gd), SEEK_SET);
    fread(&gd, sizeof(gd), 1, fp);
    int inode_index = find_free_inode(fp, sb, &gd, 0, block_size, sb->s_inodes_per_group);
    if (inode_index < 0) {
        printf("no free inodes.\n");
        return;
    }
    int block = find_free_block(fp, sb, &gd, 0, block_size, sb->s_blocks_per_group);
    if (block < 0) {
        printf("no free blocks.\n");
        return;
    }
    uint32_t inode_num = group * sb->s_inodes_per_group + inode_index;
    struct ext2_inode new_dir = {0};
    new_dir.i_mode = 0x41ED; // diretório, rwxr-xr-x
    new_dir.i_uid = 0;
    new_dir.i_gid = 0;
    new_dir.i_links_count = 2; // . e ..
    new_dir.i_size = block_size;
    new_dir.i_blocks = block_size / 512;
    new_dir.i_block[0] = block;
    new_dir.i_ctime = new_dir.i_mtime = new_dir.i_atime = time(NULL);
    uint64_t inode_offset = ((uint64_t)gd.bg_inode_table * block_size) + (inode_index * sb->s_inode_size);
    fseek(fp, inode_offset, SEEK_SET);
    fwrite(&new_dir, sizeof(struct ext2_inode), 1, fp);
    uint8_t* block_data = calloc(1, block_size);
    struct ext2_dir_entry* dot = (struct ext2_dir_entry*)block_data;
    dot->inode = inode_num;
    dot->rec_len = 12;
    dot->name_len = 1;
    dot->file_type = 2;
    strcpy(dot->name, ".");
    struct ext2_dir_entry* dotdot = (struct ext2_dir_entry*)(block_data + 12);
    dotdot->inode = *current_inode_number;
    dotdot->rec_len = block_size - 12;
    dotdot->name_len = 2;
    dotdot->file_type = 2;
    strcpy(dotdot->name, "..");
    fseek(fp, block * block_size, SEEK_SET);
    fwrite(block_data, block_size, 1, fp);
    free(block_data);
    add_directory_entry(fp, current_inode, inode_num, dirname, block_size, true);
    printf("directory '%s' created with success.\n", dirname);
}