//Fix getting directory start cluster

#ifndef _DISK_H
#define _DISK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "Log.h"

#define AUTHOR "Ryan Skelton"
#define AUTHOR_SIZE 20

#define DESC "A file system simulator"
#define DESC_SIZE 50

#define FILE_NAME "disk.dat"
#define FILE_NAME_LEN 30

#define CLUSTER_COUNT 40
#define CLUSTER_SIZE 1024
#define CLUSTER_FREE 0
#define CLUSTER_BUSY 1
#define CLUSTER_EOF CLUSTER_COUNT + 1

#define DIRECTORY_FLAG 1
#define FILE_FLAG 0

#define SUPERBLOCK_ADDR_START 0
#define ROOT_START_CLUSTER 0

#define min(x, y) ((x) < (y) ? (x) : (y))

struct Disk
{
    typedef struct
    {
        char name[AUTHOR_SIZE];
        char desc[DESC_SIZE];

        u_int32_t fat_addr_start;
        u_int32_t bitmap_addr_start;
        u_int32_t data_addr_start;
    } superblock_t;

    typedef struct
    {
        char name[FILE_NAME_LEN];
        uint32_t size;

        uint32_t start_cluster;
        uint8_t is_directory;
    } dir_entry_t;

    typedef struct
    {
        uint32_t parent_start_cluster;
        int entries_count;
        dir_entry_t *entries;
    } dir_t;

    Disk();
    ~Disk();

    superblock_t superblock;
    uint32_t fatTable[CLUSTER_COUNT];
    uint8_t bitMap[CLUSTER_COUNT];
    FILE *disk;

    dir_t *root;

    void init_superblock();
    void init_fatTable();
    void init_bitMap();
    dir_t *init_dir(uint32_t start_cluster, uint32_t parent_start_cluster);
    void createDisk();
    void init_file_system();

    void save_superblock();
    void save_fat_table();
    void save_bit_map();
    uint8_t save_dir(dir_t *&dir);

    void load_super_block();
    void load_fat_table();
    void load_bitmap();
    void load_file_system();

    dir_t *read_dir(uint32_t start_cluster_index);

    uint8_t insert_file(const char *name, FILE *file);
    uint8_t insert_dir(const char *name, dir_t *&current_working_directory, uint32_t dir_start_cluster);
    void extend_dir(dir_t *dir, dir_entry_t *entry, uint32_t curr_dir_start_cluster);

    size_t directory_size(dir_t *&dir);

    uint8_t n_free_clusters(int);
    uint32_t attain_cluster();

    uint32_t find_entry(dir_t *dir, const char *name, uint8_t flag);

    void print_dir(dir_t &dir);
    void print_fat_table();
    void print_bit_map();
    void print_file(uint32_t start_cluster, uint32_t size);
};

#endif