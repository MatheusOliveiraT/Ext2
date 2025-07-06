#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "structs.h"
#include "ext2_global.h"
#include "cat.h"
#include "path.h"

void read_and_print_block(FILE* fp, uint8_t* buffer, uint32_t block_num, uint32_t to_read, uint32_t block_size) {
    if (block_num == 0 || to_read == 0) return;
    fseek(fp, block_num * block_size, SEEK_SET);
    fread(buffer, 1, to_read, fp);
    fwrite(buffer, 1, to_read, stdout);
}

void cat_file(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_inode, uint32_t block_size) {
    uint32_t dummy_inode_num;
    struct ext2_inode* file_inode = resolve_path(fp, sb, path, current_inode, current_inode_number, block_size, &dummy_inode_num, false);
    if (!file_inode) return;
    if ((file_inode->i_mode & 0xF000) != 0x8000) {
        printf("'%s' is not a regular file.\n", path);
        free(file_inode);
        return;
    }
    uint32_t file_size = file_inode->i_size;
    uint32_t bytes_read = 0;
    uint8_t* buffer = malloc(block_size);
    if (!buffer) {
        perror("malloc");
        free(file_inode);
        return;
    }
    // Blocos diretos
    for (int i = 0; i < 12 && bytes_read < file_size; i++) {
        uint32_t to_read = (file_size - bytes_read < block_size) ? (file_size - bytes_read) : block_size;
        read_and_print_block(fp, buffer, file_inode->i_block[i], to_read, block_size);
        bytes_read += to_read;
    }
    // Bloco indireto
    if (bytes_read < file_size && file_inode->i_block[12]) {
        uint32_t entries = block_size / sizeof(uint32_t);
        uint32_t* indirect_block = malloc(block_size);
        fseek(fp, file_inode->i_block[12] * block_size, SEEK_SET);
        fread(indirect_block, sizeof(uint32_t), entries, fp);
        for (uint32_t i = 0; i < entries && bytes_read < file_size; i++) {
            if (indirect_block[i] == 0) continue;
            uint32_t to_read = (file_size - bytes_read < block_size) ? (file_size - bytes_read) : block_size;
            read_and_print_block(fp, buffer, indirect_block[i], to_read, block_size);
            bytes_read += to_read;
        }
        free(indirect_block);
    }
    // Bloco duplamente indireto
    if (bytes_read < file_size && file_inode->i_block[13]) {
        uint32_t entries = block_size / sizeof(uint32_t);
        uint32_t* double_indirect = malloc(block_size);
        fseek(fp, file_inode->i_block[13] * block_size, SEEK_SET);
        fread(double_indirect, sizeof(uint32_t), entries, fp);
        for (uint32_t i = 0; i < entries && bytes_read < file_size; i++) {
            if (double_indirect[i] == 0) continue;
            uint32_t* indirect = malloc(block_size);
            fseek(fp, double_indirect[i] * block_size, SEEK_SET);
            fread(indirect, sizeof(uint32_t), entries, fp);
            for (uint32_t j = 0; j < entries && bytes_read < file_size; j++) {
                if (indirect[j] == 0) continue;
                uint32_t to_read = (file_size - bytes_read < block_size) ? (file_size - bytes_read) : block_size;
                read_and_print_block(fp, buffer, indirect[j], to_read, block_size);
                bytes_read += to_read;
            }
            free(indirect);
        }
        free(double_indirect);
    }
    free(buffer);
    free(file_inode);
    printf("\n");
}