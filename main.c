#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BASE_OFFSET 1024
#define EXT2_SUPER_MAGIC 0xEF53

char dir_atual[256] = "/";

struct ext2_super_block {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime; 
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks; 
    uint16_t s_reserved_gdt_blocks;
    uint8_t  s_journal_uuid[16];      
    uint32_t s_journal_inum;          
    uint32_t s_journal_dev;           
    uint32_t s_last_orphan;           
    uint32_t s_hash_seed[4];          
    uint8_t  s_def_hash_version;      
    uint8_t  s_jnl_backup_type;
    uint16_t s_desc_size;            
    uint32_t s_default_mount_opts;    
    uint32_t s_first_meta_bg; 
};

struct ext2_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
};

void print_superblock(struct ext2_super_block* sb) {
    printf("inodes count: %u\n", sb->s_inodes_count);
    printf("blocks count: %u\n", sb->s_blocks_count);
    printf("reserved blocks count: %u\n", sb->s_r_blocks_count);
    printf("free blocks count: %u\n", sb->s_free_blocks_count);
    printf("free inodes count: %u\n", sb->s_free_inodes_count);
    printf("first data block: %u\n", sb->s_first_data_block);
    printf("block size: %u\n", 1024 << sb->s_log_block_size);
    printf("fragment size: %u\n", sb->s_log_frag_size);
    printf("blocks per group: %u\n", sb->s_blocks_per_group);
    printf("fragments per group: %u\n", sb->s_frags_per_group);
    printf("inodes per group: %u\n", sb->s_inodes_per_group);
    printf("mount time: %u\n", sb->s_mtime);
    printf("write time: %u\n", sb->s_wtime);
    printf("mount count: %u\n", sb->s_mnt_count);
    printf("max mount count: %u\n", sb->s_max_mnt_count);
    printf("magic signature: 0x%x\n", sb->s_magic);
    printf("file system state: %u\n", sb->s_state);
    printf("errors: %u\n", sb->s_errors);
    printf("minor revision level: %u\n", sb->s_minor_rev_level);
    printf("last check time: %u\n", sb->s_lastcheck);
    printf("check interval: %u\n", sb->s_checkinterval);
    printf("creator OS: %u\n", sb->s_creator_os);
    printf("revision level: %u\n", sb->s_rev_level);
    printf("default reserved UID: %u\n", sb->s_def_resuid);
    printf("default reserved GID: %u\n", sb->s_def_resgid);
    printf("first non-reserved inode: %u\n", sb->s_first_ino);
    printf("inode size: %u\n", sb->s_inode_size);
    printf("block group number: %u\n", sb->s_block_group_nr);
    printf("compatible features: 0x%x\n", sb->s_feature_compat);
    printf("incompatible features: 0x%x\n", sb->s_feature_incompat);
    printf("readonly-compatible features: 0x%x\n", sb->s_feature_ro_compat);
    printf("filesystem UUID: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", sb->s_uuid[i]);
    }
    printf("\n");
    printf("volume name: %.16s\n", sb->s_volume_name);
    printf("last mounted on: %.64s\n", sb->s_last_mounted);
    printf("algorithm usage bitmap: 0x%x\n", sb->s_algorithm_usage_bitmap);
    printf("prealloc blocks: %u\n", sb->s_prealloc_blocks);
    printf("prealloc dir blocks: %u\n", sb->s_prealloc_dir_blocks);
    printf("reserved GDT blocks: %u\n", sb->s_reserved_gdt_blocks);
    printf("journal UUID: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", sb->s_journal_uuid[i]);
    }
    printf("\n");
    printf("journal inode: %u\n", sb->s_journal_inum);
    printf("journal device: %u\n", sb->s_journal_dev);
    printf("last orphan: %u\n", sb->s_last_orphan);
    printf("hash seed: ");
    for (int i = 0; i < 4; i++) {
        printf("%08x", sb->s_hash_seed[i]);
    }
    printf("\n");
    printf("default hash version: %u\n", sb->s_def_hash_version);
    printf("journal backup type: %u\n", sb->s_jnl_backup_type);
    printf("descriptor size: %u\n", sb->s_desc_size);
    printf("default mount options: 0x%x\n", sb->s_default_mount_opts);
    printf("first meta block group: %u\n", sb->s_first_meta_bg);
}

int print_groups(FILE* fp, struct ext2_super_block* sb) {
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint32_t total_blocks = (sb->s_blocks_count + sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;
    uint32_t desc_size = sb->s_desc_size ? sb->s_desc_size : 32;

    uint64_t offset;
    if (block_size == 1024) { 
        offset = 2 * block_size;
    } else { 
        offset = block_size;
    }

    fseek(fp, BASE_OFFSET, SEEK_SET);
    for (uint32_t i = 0; i < total_blocks; i++) {
        struct ext2_group_desc gd;
        
        fseek(fp, offset + i * desc_size, SEEK_SET);
        fread(&gd, sizeof(gd), 1, fp);

        printf("Block Group Descriptor %u:\n", i);
        printf("block bitmap: %u\n", gd.bg_block_bitmap);
        printf("inode bitmap: %u\n", gd.bg_inode_bitmap);
        printf("inode table: %u\n", gd.bg_inode_table);
        printf("free blocks count: %u\n", gd.bg_free_blocks_count);
        printf("free inodes count: %u\n", gd.bg_free_inodes_count);
        printf("used dirs count: %u\n\n", gd.bg_used_dirs_count);
    }
    fseek(fp, BASE_OFFSET, SEEK_SET);

    return 0;
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

    while(1) {
        printf("[%s]$> ", dir_atual);

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
            } else {
                printf("invalid sintax.\n");
            }
        } else if (strcmp(token, "exit") == 0) {
            break;
        } else {
            printf("invalid sintax.\n");
        }
    }

    fclose(fp);
    return 0;
}