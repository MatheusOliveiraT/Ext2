#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "print.h"

void format_permissions(uint16_t mode, char* out) {
    out[0] = (mode & 0xF000) == 0x4000 ? 'd' :
             (mode & 0xF000) == 0x8000 ? 'f' :
             (mode & 0xF000) == 0xA000 ? 'l' : '?';
    for (int i = 0; i < 3; i++) {
        out[1 + i*3 + 0] = (mode & (1 << (8 - i*3))) ? 'r' : '-';
        out[1 + i*3 + 1] = (mode & (1 << (7 - i*3))) ? 'w' : '-';
        out[1 + i*3 + 2] = (mode & (1 << (6 - i*3))) ? 'x' : '-';
    }
    out[10] = '\0';
}

void format_time(uint32_t epoch, char* out, size_t out_size) {
    time_t raw = epoch;
    struct tm* tm_info = localtime(&raw);
    strftime(out, out_size, "%d/%m/%Y %H:%M", tm_info);
}

void info(FILE* fp, struct ext2_super_block* sb) {
    fseek(fp, 0, SEEK_END);
    long image_size = ftell(fp);
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint32_t groups_count = (sb->s_blocks_count + sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;
    uint32_t inodetable_size = (sb->s_inodes_per_group * sb->s_inode_size) / block_size;
    printf("Volume name.....: %.16s\n", sb->s_volume_name);
    printf("Image size......: %ld bytes\n", image_size);
    printf("Free space......: %u KiB\n", sb->s_free_blocks_count * block_size / 1024);
    printf("Free inodes.....: %u\n", sb->s_free_inodes_count);
    printf("Free blocks.....: %u\n", sb->s_free_blocks_count);
    printf("Block size......: %u bytes\n", block_size);
    printf("Inode size......: %u bytes\n", sb->s_inode_size);
    printf("Groups count....: %u\n", groups_count);
    printf("Groups size.....: %u blocks\n", sb->s_blocks_per_group);
    printf("Groups inodes...: %u inodes\n", sb->s_inodes_per_group);
    printf("Inodetable size.: %u blocks\n", inodetable_size);
}

void list_directory(FILE *fp, struct ext2_super_block* sb, struct ext2_inode* inode) {
    if (!inode || !(inode->i_mode & 0x4000)) {
        printf("inode is not a directory.\n");
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
        while (offset < (uint32_t)(1024 << sb->s_log_block_size)) {
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

void print_groups(FILE* fp, struct ext2_super_block* sb) {
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint32_t total_blocks = (sb->s_blocks_count + sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;
    uint32_t desc_size = sb->s_desc_size ? sb->s_desc_size : 32;
    uint64_t offset;
    if (block_size == 1024) { 
        offset = 2 * block_size;
    } else { 
        offset = block_size;
    }
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
}   

void print_inode(FILE* fp, struct ext2_super_block* sb, uint32_t inode_num) {
    struct ext2_inode* inode = get_inode(fp, sb, inode_num);
    printf("file format and access rights: 0x%x\n", inode->i_mode);
    printf("user id: %u\n", inode->i_uid);
    printf("lower 32-bit file size: %u\n", inode->i_size);
    printf("access time: %u\n", inode->i_atime);
    printf("creation time: %u\n", inode->i_ctime);
    printf("modification time: %u\n", inode->i_mtime);
    printf("deletion time: %u\n", inode->i_dtime);
    printf("group id: %u\n", inode->i_gid);
    printf("link count inode: %u\n", inode->i_links_count);
    printf("512-bytes blocks: %u\n", inode->i_blocks);
    printf("ext2 flags: %u\n", inode->i_flags);
    printf("reserved (Linux): %u\n", inode->i_osd1);
    for (int i = 0; i < 15; i++) {
        printf("pointer[%d]: %u\n", i, inode->i_block[i]);
    }
    printf("file version (nfs): %u\n", inode->i_generation);
    printf("block number extended attributes: %u\n", inode->i_file_acl);
    printf("higher 32-bit file size: %u\n", inode->i_dir_acl);
    printf("location file fragment: %u\n", inode->i_faddr);
    free(inode);
}

void print_rootdir(FILE* fp, struct ext2_super_block* sb, uint32_t block_size) {
    printf("--- root directory (/) ---\n");
    print_dir(fp, sb, "/", current_inode, current_inode_number, block_size);
}

void print_dir(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_dir_inode, uint32_t current_dir_inode_num, uint32_t block_size) {
    uint32_t target_inode_num;
    struct ext2_inode* dir_inode = resolve_path(fp, sb, path, current_dir_inode, current_dir_inode_num, block_size, &target_inode_num, false);
    if (!dir_inode) {
        printf("directory '%s' not found or is invalid.\n", path);
        return;
    }
    if ((dir_inode->i_mode & 0xF000) != 0x4000) {
        printf("'%s' is not a directory.\n", path);
        free(dir_inode);
        return;
    }
    printf("--- directory: %s (inode: %u) ---\n", path, target_inode_num);
    printf("%-10s %-10s %-10s %-10s %-20s %s\n", "inode", "type", "name len", "rec len", "name", "permissions");
    printf("--------------------------------------------------------------------------------\n");
    uint8_t* block_buffer = malloc(block_size);
    if (!block_buffer) {
        perror("malloc");
        free(dir_inode);
        return;
    }
    for (int i = 0; i < 12 && dir_inode->i_block[i]; i++) {
        fseek(fp, dir_inode->i_block[i] * block_size, SEEK_SET);
        fread(block_buffer, block_size, 1, fp);
        uint32_t offset = 0;
        while (offset < block_size) {
            struct ext2_dir_entry* entry = (struct ext2_dir_entry*)(block_buffer + offset);
            if (entry->rec_len == 0) break;
            char name[256] = {0};
            memcpy(name, entry->name, entry->name_len);
            name[entry->name_len] = '\0';
            struct ext2_inode* entry_inode = get_inode(fp, sb, entry->inode);
            char type_char = '?';
            char perms_str[11] = "----------";
            if (entry_inode) {
                if ((entry_inode->i_mode & 0xF000) == 0x8000) type_char = '-';
                else if ((entry_inode->i_mode & 0xF000) == 0x4000) type_char = 'd';
                else if ((entry_inode->i_mode & 0xF000) == 0xA000) type_char = 'l';
                format_permissions(entry_inode->i_mode, perms_str);
                free(entry_inode);
            }
            printf("%-10u %-10c %-10u %-10u %-20s %s\n",
                   entry->inode, type_char, entry->name_len, entry->rec_len, name, perms_str);
            offset += entry->rec_len;
        }
    }
    free(block_buffer);
    free(dir_inode);
}

int get_bit(FILE* fp, uint32_t bitmap_block, uint32_t bit_index, uint32_t block_size) {
    uint8_t* bitmap_buffer = malloc(block_size);
    if (!bitmap_buffer) { perror("malloc"); return -1; }
    read_block(fp, bitmap_block, block_size, bitmap_buffer);
    int byte_index = bit_index / 8;
    int bit_in_byte = bit_index % 8;
    int bit_value = (bitmap_buffer[byte_index] >> bit_in_byte) & 1;
    free(bitmap_buffer);
    return bit_value;
}

int read_block(FILE* fp, uint32_t block_num, uint32_t block_size, uint8_t* buffer) {
    if (!fp || !buffer) {
        fprintf(stderr, "error: invalid file pointer or buffer in read_block.\n");
        return -1;
    }
    off_t offset = (off_t)block_num * block_size;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        perror("error seeking to block");
        return -1;
    }
    if (fread(buffer, 1, block_size, fp) != block_size) {
        perror("error reading block");
        return -1;
    }
    return 0;
}

struct ext2_group_desc* get_group_descriptor(FILE* fp, struct ext2_super_block* sb, uint32_t group_id) {
    if (!fp || !sb) {
        fprintf(stderr, "error: invalid file pointer or superblock in get_group_descriptor.\n");
        return NULL;
    }
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint32_t bgdt_start_block = sb->s_first_data_block + 1;
    uint32_t num_block_groups = (sb->s_blocks_count + sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;
    if (group_id >= num_block_groups) {
        fprintf(stderr, "error: group ID %u is out of bounds (max %u).\n", group_id, num_block_groups - 1);
        return NULL;
    }
    uint32_t gd_size = sizeof(struct ext2_group_desc);
    uint32_t gd_offset_in_gdt = group_id * gd_size;
    uint32_t gd_block_num = bgdt_start_block + (gd_offset_in_gdt / block_size);
    uint32_t gd_offset_within_block = gd_offset_in_gdt % block_size;
    uint8_t* block_buffer = malloc(block_size);
    if (!block_buffer) {
        perror("malloc");
        return NULL;
    }
    if (read_block(fp, gd_block_num, block_size, block_buffer) != 0) {
        fprintf(stderr, "error: could not read block %u for group descriptor.\n", gd_block_num);
        free(block_buffer);
        return NULL;
    }
    struct ext2_group_desc* gd = malloc(gd_size);
    if (!gd) {
        perror("malloc");
        free(block_buffer);
        return NULL;
    }
    memcpy(gd, block_buffer + gd_offset_within_block, gd_size);
    free(block_buffer);
    return gd;
}

void print_inode_bitmap(FILE* fp, struct ext2_super_block* sb, uint32_t group_id, uint32_t block_size) {
    struct ext2_group_desc* gd = get_group_descriptor(fp, sb, group_id);
    if (!gd) {
        printf("error: could not get group descriptor for group %u.\n", group_id);
        return;
    }
    printf("--- inode bitmap for block group %u (block: %u) ---\n", group_id, gd->bg_inode_bitmap);
    uint32_t inodes_per_group = sb->s_inodes_per_group;
    if (group_id == (uint32_t)(sb->s_block_group_nr - 1)) {
        inodes_per_group = sb->s_inodes_count - (group_id * sb->s_inodes_per_group);
    }
    uint8_t* bitmap_buffer = malloc(block_size);
    if (!bitmap_buffer) {
        perror("malloc");
        free(gd);
        return;
    }
    read_block(fp, gd->bg_inode_bitmap, block_size, bitmap_buffer);
    for (uint32_t i = 0; i < inodes_per_group; i++) {
        int byte_index = i / 8;
        int bit_in_byte = i % 8;
        int bit_value = (bitmap_buffer[byte_index] >> bit_in_byte) & 1;
        printf("%d", bit_value);
        if ((i + 1) % 8 == 0) printf(" ");
        if ((i + 1) % 64 == 0) printf("\n");
    }
    printf("\n");
    free(bitmap_buffer);
    free(gd);
}

void print_block_bitmap(FILE* fp, struct ext2_super_block* sb, uint32_t group_id, uint32_t block_size) {
    struct ext2_group_desc* gd = get_group_descriptor(fp, sb, group_id);
    if (!gd) {
        printf("error: could not get group descriptor for group %u.\n", group_id);
        return;
    }
    printf("--- block bitmap for block group %u (block: %u) ---\n", group_id, gd->bg_block_bitmap);
    uint32_t blocks_per_group = sb->s_blocks_per_group;
    if (group_id == (uint32_t)(sb->s_block_group_nr - 1)) {
        blocks_per_group = sb->s_blocks_count - (group_id * sb->s_blocks_per_group);
    }
    uint8_t* bitmap_buffer = malloc(block_size);
    if (!bitmap_buffer) {
        perror("malloc");
        free(gd);
        return;
    }
    read_block(fp, gd->bg_block_bitmap, block_size, bitmap_buffer);
    for (uint32_t i = 0; i < blocks_per_group; i++) {
        int byte_index = i / 8;
        int bit_in_byte = i % 8;
        int bit_value = (bitmap_buffer[byte_index] >> bit_in_byte) & 1;
        printf("%d", bit_value);
        if ((i + 1) % 8 == 0) printf(" ");
        if ((i + 1) % 64 == 0) printf("\n");
    }
    printf("\n");
    free(bitmap_buffer);
    free(gd);
}

void print_block_content(FILE* fp, uint32_t block_num, uint32_t block_size) {
    if (block_num == 0) {
        printf("cannot print content of block 0 (invalid block number).\n");
        return;
    }
    printf("--- content of block %u (size: %u bytes) ---\n", block_num, block_size);
    uint8_t* buffer = malloc(block_size);
    if (!buffer) {
        perror("malloc");
        return;
    }
    fseek(fp, block_num * block_size, SEEK_SET);
    fread(buffer, block_size, 1, fp);
    for (uint32_t i = 0; i < block_size; i++) {
        printf("%02x ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf(" | ");
            for (uint32_t j = i - 15; j <= i; j++) {
                if (buffer[j] >= 32 && buffer[j] <= 126) {
                    printf("%c", buffer[j]);
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }
    }
    if (block_size % 16 != 0) {
        int remaining = 16 - (block_size % 16);
        for (int i = 0; i < remaining; i++) {
            printf("   ");
        }
        printf(" | ");
        for (uint32_t j = block_size - (block_size % 16); j < block_size; j++) {
            if (buffer[j] >= 32 && buffer[j] <= 126) {
                printf("%c", buffer[j]);
            } else {
                printf(".");
            }
        }
        printf("\n");
    }
    free(buffer);
}

void attr_file(FILE* fp, struct ext2_super_block* sb, const char* path, struct ext2_inode* current_inode, uint32_t block_size) {
    uint32_t dummy_inode_num;
    struct ext2_inode* inode = resolve_path(fp, sb, path, current_inode, current_inode_number, block_size, &dummy_inode_num, false);
    if (!inode) return;
    char perms[11];
    format_permissions(inode->i_mode, perms);
    float size_kib = inode->i_size / 1024.0f;
    char time_str[20];
    format_time(inode->i_mtime, time_str, sizeof(time_str));
    printf("permissions  uid  gid      size      last modified at\n");
    printf("%-10s   %-4u %-4u  %5.1f KiB    %s\n", perms, inode->i_uid, inode->i_gid, size_kib, time_str);
    free(inode);
}