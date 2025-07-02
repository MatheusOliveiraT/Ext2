#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "structs.c"
#include "print.c"
#include "inode.c"

#define BASE_OFFSET 1024
#define EXT2_SUPER_MAGIC 0xEF53

char current_path[256] = "/";
struct ext2_inode *current_inode;

void list_directory(FILE *fp, struct ext2_super_block* sb, struct ext2_inode* inode) {
    if (!inode || !(inode->i_mode & 0x4000)) {
        printf("Inode não é um diretório.\n");
        return;
    }
    uint8_t *block = malloc(1024 << sb->s_log_block_size);
    if (!block) {
        perror("malloc");
        return;
    }
    for (int i = 0; i < 12; i++) {
        if (inode->i_block[i] == 0)
            continue;
        fseek(fp, inode->i_block[i] * 1024 << sb->s_log_block_size, SEEK_SET);
        fread(block, 1024 << sb->s_log_block_size, 1, fp);
        uint32_t offset = 0;
        while (offset < 1024 << sb->s_log_block_size) {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(block + offset);
            if (entry->inode != 0) {
                char name[256] = {0};
                memcpy(name, entry->name, entry->name_len);
                name[entry->name_len] = '\0';
                printf("%s\n", name);
                printf("inode: %u\n", entry->inode);
                printf("record lenght: %u\n", entry->rec_len);
                printf("name lenght: %u\n", entry->name_len);
                printf("file type: %u\n\n", entry->file_type);
            }
            if (entry->rec_len == 0) break;
            offset += entry->rec_len;
        }
    }
    free(block);
}

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
    struct ext2_inode* destino = resolve_path(fp, sb, path, current_inode, block_size);
    if (!destino) return;
    if ((destino->i_mode & 0xF000) != 0x4000) {
        printf("'%s' is not an directory.\n", path);
        free(destino);
        return;
    }
    free(current_inode);
    current_inode = destino;
    update_path(path);
}

void read_and_print_block(FILE* fp, uint8_t* buffer, uint32_t block_num, uint32_t to_read, uint32_t block_size) {
    if (block_num == 0 || to_read == 0) return;
    fseek(fp, block_num * block_size, SEEK_SET);
    fread(buffer, 1, to_read, fp);
    fwrite(buffer, 1, to_read, stdout);
}

void cat_file(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_inode, uint32_t block_size) {
    struct ext2_inode* file_inode = resolve_path(fp, sb, path, current_inode, block_size);
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

int main(int argc, char *argv[]) {
    FILE *fp;
    struct ext2_super_block sb;
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <imagem ext2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Erro ao abrir imagem");
        exit(EXIT_FAILURE);
    }
    fseek(fp, BASE_OFFSET, SEEK_SET);
    fread(&sb, sizeof(struct ext2_super_block), 1, fp);
    if (sb.s_magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Não é uma imagem ext2 válida (magic errado: 0x%x)\n", sb.s_magic);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    current_inode = get_inode(fp, &sb, 2);
    while(1) {
        printf("[%s]$> ", current_path);
        char entrada[256];
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            break;
        }
        entrada[strcspn(entrada, "\n")] = '\0';
        char *token = strtok(entrada, " ");
        if (token == NULL) continue;
        if (strcmp(token, "print") == 0) {
            token = strtok(NULL, " ");
            if (token && strcmp(token, "superblock") == 0) {
                print_superblock(&sb);
            } else if (token && strcmp(token, "groups") == 0) {
                print_groups(fp, &sb);
            } else if (token && strcmp(token, "inode") == 0) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("invalid sintax.\n");
                    continue;
                }
                char *endptr;
                long num = strtol(token, &endptr, 10);
                if (token && num) {
                    print_inode(fp, &sb, num);
                } else {
                    printf("invalid sintax.\n");
                }
            } else {
                printf("invalid sintax.\n");
            }
        } else if (strcmp(token, "info") == 0) {
            info(fp, &sb);
        } else if (strcmp(token, "ls") == 0) {
            list_directory(fp, &sb, current_inode);
        } else if (strcmp(token, "cd") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax.\n");
                continue;
            }
            change_directory(fp, &sb, token, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "cat") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax.\n");
                continue;
            }
            cat_file(fp, &sb, token, current_inode, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "exit") == 0) {
            break;
        } else {
            printf("invalid command.\n");
        }
    }
    fclose(fp);
    return 0;
}