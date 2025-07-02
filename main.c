#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "structs.c"
#include "print.c"
#include "inode.c"
#include "path.c"
#include "cat.c"

#define BASE_OFFSET 1024
#define EXT2_SUPER_MAGIC 0xEF53

char current_path[256] = "/";
struct ext2_inode *current_inode;

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
        } else if (strcmp(token, "attr") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax.\n");
                continue;
            }
            attr_file(fp, &sb, token, current_inode, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "exit") == 0) {
            break;
        } else {
            printf("invalid command.\n");
        }
    }
    fclose(fp);
    return 0;
}