#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "structs.h"
#include "inode.h"
#include "ext2_global.h"

struct ext2_inode* resolve_path(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* start_inode, uint32_t start_inode_number, uint32_t block_size, uint32_t* final_inode_number, bool update_global_context) {
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));
    path_copy[sizeof(path_copy) - 1] = '\0';
    struct ext2_inode* current_resolved_inode;
    uint32_t current_resolved_inode_num;
    if (path[0] == '/') {
        current_resolved_inode = get_inode(fp, sb, 2);
        current_resolved_inode_num = 2;
    } else {
        current_resolved_inode = malloc(sizeof(struct ext2_inode));
        if (!current_resolved_inode) {
            perror("malloc");
            return NULL;
        }
        memcpy(current_resolved_inode, start_inode, sizeof(struct ext2_inode));
        current_resolved_inode_num = start_inode_number;
    }
    char* token = strtok(path_copy, "/");
    if (path[0] == '/' && (token == NULL || strcmp(token, "") == 0)) {
    } else {
        while (token != NULL) {
            if ((current_resolved_inode->i_mode & 0xF000) != 0x4000) {
                printf("'%s' is not a directory.\n", token);
                free(current_resolved_inode);
                return NULL;
            }
            int found = 0;
            uint8_t* block = malloc(block_size);
            if (!block) {
                perror("malloc");
                free(current_resolved_inode);
                return NULL;
            }
            for (int i = 0; i < 12 && current_resolved_inode->i_block[i]; i++) {
                fseek(fp, current_resolved_inode->i_block[i] * block_size, SEEK_SET);
                fread(block, block_size, 1, fp);
                uint32_t offset = 0;
                while (offset < block_size) {
                    struct ext2_dir_entry* entry = (struct ext2_dir_entry*)(block + offset);
                    char name[256] = {0};
                    memcpy(name, entry->name, entry->name_len);
                    name[entry->name_len] = '\0';
                    if (strcmp(name, token) == 0) {
                        found = 1;
                        free(current_resolved_inode); 
                        current_resolved_inode = get_inode(fp, sb, entry->inode);
                        current_resolved_inode_num = entry->inode;
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
                free(current_resolved_inode);
                return NULL;
            }
            token = strtok(NULL, "/");
        }
    }
    if (update_global_context) {
        if (current_inode != NULL && current_inode != current_resolved_inode) {
            free(current_inode);
        }
        current_inode = current_resolved_inode;
        current_inode_number = current_resolved_inode_num;
    }
    if (final_inode_number) {
        *final_inode_number = current_resolved_inode_num;
    }
    return current_resolved_inode;
}