#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "structs.h"
#include "cd.h"
#include "print.h"
#include "inode.h"
#include "path.h"
#include "cat.h"
#include "touch.h"
#include "ext2_global.h"
#include "rm.h"

#define BASE_OFFSET 1024
#define EXT2_SUPER_MAGIC 0xEF53

int main(int argc, char *argv[]) {
    FILE *fp;
    struct ext2_super_block sb;
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <imagem ext2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    fp = fopen(argv[1], "rb+");
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
    current_inode_number = 2;
    current_inode = get_inode(fp, &sb, current_inode_number);
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
            if (token == NULL) {
                printf("invalid sintax. expected: print [ superblock | groups | inode | rootdir | dir | inodebitmap | blockbitmap | attr | block ]\n");
                continue;
            }
            if (token && strcmp(token, "superblock") == 0) {
                print_superblock(&sb);
            } else if (token && strcmp(token, "groups") == 0) {
                print_groups(fp, &sb);
            } else if (token && strcmp(token, "inode") == 0) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("invalid sintax. expected: print inode <inode_num>\n");
                    continue;
                }
                char *endptr;
                long num = strtol(token, &endptr, 10);
                if (endptr != token) {
                    print_inode(fp, &sb, num);
                } else {
                    printf("invalid sintax. expected: print inode <inode_num>\n");
                }
            } else if (strcmp(token, "rootdir") == 0) {
                print_rootdir(fp, &sb, 1024 << sb.s_log_block_size);
            } else if (strcmp(token, "dir") == 0) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("invalid sintax. expected: print dir <path>\n");
                    continue;
                }
                print_dir(fp, &sb, token, current_inode, current_inode_number, 1024 << sb.s_log_block_size);
            } else if (strcmp(token, "inodebitmap") == 0) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("invalid sintax. expected: print inodebitmap <group_id>\n");
                    continue;
                }
                char *endptr;
                long group_id = strtol(token, &endptr, 10);
                if (endptr != token) {
                    print_inode_bitmap(fp, &sb, (uint32_t)group_id, 1024 << sb.s_log_block_size);
                } else {
                    printf("invalid sintax. expected: print inodebitmap <group_id>.\n");
                }
            } else if (strcmp(token, "blockbitmap") == 0) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("invalid sintax. expected: print blockbitmap <group_id>\n");
                    continue;
                }
                char *endptr;
                long group_id = strtol(token, &endptr, 10);
                if (endptr != token) {
                    print_block_bitmap(fp, &sb, (uint32_t)group_id, 1024 << sb.s_log_block_size);
                } else {
                    printf("invalid sintax. expected: print blockbitmap <group_id>.\n");
                }
            } else if (strcmp(token, "attr") == 0) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("invalid sintax. expected: print attr <file_path>\n");
                    continue;
                }
                attr_file(fp, &sb, token, current_inode, 1024 << sb.s_log_block_size);
            } else if (strcmp(token, "block") == 0) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("invalid sintax. expected: print block <block_number>\n");
                    continue;
                }
                char *endptr;
                long block_num = strtol(token, &endptr, 10);
                if (endptr != token && block_num > 0) {
                    print_block_content(fp, (uint32_t)block_num, 1024 << sb.s_log_block_size);
                } else {
                    printf("invalid sintax. expected: print block <block_number>\n");
                }
            } else {
                printf("invalid sintax. expected: print [ superblock | groups | inode | rootdir | dir | inodebitmap | blockbitmap | attr | block ]\n");
            }
        } else if (strcmp(token, "info") == 0) {
            info(fp, &sb);
        } else if (strcmp(token, "ls") == 0) {
            list_directory(fp, &sb, current_inode);
        } else if (strcmp(token, "pwd") == 0) {
            printf("%s\n", current_path);
        } else if (strcmp(token, "cd") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax. expected: cd <path>\n");
                continue;
            }
            change_directory(fp, &sb, token, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "cat") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax. expected: cat <filename>\n");
                continue;
            }
            cat_file(fp, &sb, token, current_inode, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "attr") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax. expected: attr <file | dir>\n");
                continue;
            }
            attr_file(fp, &sb, token, current_inode, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "touch") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax. expected: touch <filename>\n");
                continue;
            }
            touch(fp, &sb, current_inode, token, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "mkdir") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax. expected: mkdir <dir>\n");
                continue;
            }
            mkdir_ext2(fp, &sb, current_inode, token, &current_inode_number, 1024 << sb.s_log_block_size);
        } else if (strcmp(token, "rm") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax. expected: rm <filename>\n");
                continue;
            }
            remove_entry(fp, &sb, token, 0);
        } else if (strcmp(token, "rmdir") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                printf("invalid sintax. expected: rmdir <dir>\n");
                continue;
            }
            remove_entry(fp, &sb, token, 1);
        } else if (strcmp(token, "cp") == 0) {
            char* src = strtok(NULL, " ");
            char* dst = strtok(NULL, " ");
            if (src && dst)
                cp_file(fp, &sb, current_inode, src, dst, 1024 << sb.s_log_block_size);
            else
                printf("invalid sintax. expected: cp <source_path> <target_path>\n");
        }
        else if (strcmp(token, "rename") == 0) {
            char* oldname = strtok(NULL, " ");
            char* newname = strtok(NULL, " ");
            if (oldname && newname)
                rename_entry(fp, current_inode, oldname, newname, 1024 << sb.s_log_block_size);
            else
                printf("invalid sintax. expected: rename <filename> <newfilename>\n");
        } else if (strncmp(token, "echo", 4) == 0) {
            const char* rest = token + 5;
            char content[1024] = {0}, filename[256] = {0};
            if (sscanf(rest, "\"%[^\"]\" > %s", content, filename) == 2) {
                echo_to_file(fp, &sb, current_inode, current_inode_number, content, filename, 1024 << sb.s_log_block_size);
            } else {
                printf("invalid sintax. expected: echo \"text\" > <filename>\n");
            }
        } else if (strcmp(token, "exit") == 0) {
            break;
        } else {
            printf("invalid command.\n");
        }
    }
    fclose(fp);
    return 0;
}