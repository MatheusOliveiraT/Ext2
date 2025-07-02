#include <stdio.h>
#include "structs.c"

#pragma once

struct ext2_inode* resolve_path(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* start_inode, uint32_t block_size) {
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));
    path_copy[sizeof(path_copy) - 1] = '\0';
    struct ext2_inode* current;
    if (path[0] == '/') {
        current = get_inode(fp, sb, 2);
    } else {
        current = malloc(sizeof(struct ext2_inode));
        memcpy(current, start_inode, sizeof(struct ext2_inode));
    }
    char* token = strtok(path_copy, "/");
    while (token != NULL) {
        if ((current->i_mode & 0xF000) != 0x4000) {
            printf("'%s' is not an directory.\n", token);
            free(current);
            return NULL;
        }
        int found = 0;
        uint8_t* block = malloc(block_size);
        if (!block) {
            perror("malloc");
            free(current);
            return NULL;
        }
        for (int i = 0; i < 12 && current->i_block[i]; i++) {
            fseek(fp, current->i_block[i] * block_size, SEEK_SET);
            fread(block, block_size, 1, fp);
            uint32_t offset = 0;
            while (offset < block_size) {
                struct ext2_dir_entry* entry = (struct ext2_dir_entry*)(block + offset);
                char name[256] = {0};
                memcpy(name, entry->name, entry->name_len);
                name[entry->name_len] = '\0';
                if (strcmp(name, token) == 0) {
                    found = 1;
                    struct ext2_inode* next = get_inode(fp, sb, entry->inode);
                    free(current);
                    current = next;
                    break;
                }
                if (entry->rec_len == 0) break;
                offset += entry->rec_len;
            }
            if (found) break;
        }
        free(block);
        if (!found) {
            printf("'%s' not found.\n", token);
            free(current);
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    return current;
}