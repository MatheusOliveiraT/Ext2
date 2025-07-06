#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "structs.h"
#include "path.h"
#include "cd.h"
#include "ext2_global.h"

void update_path(const char* path) {
    if (strcmp(path, "/") == 0) {
        strcpy(current_path, "/");
    }
    else if (strcmp(path, "..") == 0) {
        if (strcmp(current_path, "/") == 0) return;
        char* last_slash = strrchr(current_path, '/');
        if (last_slash != NULL) {
            *last_slash = '\0'; 
            if (strlen(current_path) == 0) {
                strcpy(current_path, "/");
            }
        }
    }
    else if (strcmp(path, ".") == 0) {
        return;
    }
    else if (path[0] == '/') {
        strncpy(current_path, path, sizeof(current_path) - 1);
        current_path[sizeof(current_path) - 1] = '\0';
    }
    else {
        if (strcmp(current_path, "/") != 0) {
            strncat(current_path, "/", sizeof(current_path) - strlen(current_path) - 1);
        }
        strncat(current_path, path, sizeof(current_path) - strlen(current_path) - 1);
    }
}

void change_directory(FILE* fp, struct ext2_super_block* sb, const char* path, uint32_t block_size) {
    uint32_t destino_inode_number;
    struct ext2_inode* destino = resolve_path(fp, sb, path, current_inode, current_inode_number, block_size, &destino_inode_number, false);
    if (!destino) return;
    if ((destino->i_mode & 0xF000) != 0x4000) {
        printf("'%s' is not an directory.\n", path);
        free(destino);
        return;
    }
    free(current_inode);
    current_inode = destino;
    current_inode_number = destino_inode_number;
    update_path(path);
}