#include "Disk.h"

Disk::Disk()
{
    if (access(FILE_NAME, F_OK))
        init_file_system();
    else
        load_file_system();
}

Disk::~Disk()
{
    free(root);
    fclose(disk);
    free(disk);
}

void Disk::init_file_system()
{

    init_superblock();
    //LOG(Log::INFO, "superblock has been initialised");
    init_fatTable();
    //LOG(Log::INFO, "fat table has been initialised");
    init_bitMap();
    //LOG(Log::INFO, "bit Map has been initialised");
    root = init_dir(ROOT_START_CLUSTER, ROOT_START_CLUSTER);
    //LOG(Log::INFO, "root directory has been initialised");
    createDisk();
    //LOG(Log::INFO, "disk has been initialised");
    save_superblock();
    //LOG(Log::INFO, "superblock has been written to the disk");

    if (!save_dir(root))
    {
        //LOG(Log::ERROR, "Not enough cluster space");
        exit(1);
    }
    //LOG(Log::INFO, "root directory has been written to the disk");

    LOG(Log::INFO, "file system has been initialised");
    LOG(Log::INFO, "file system has been saved");
}

void Disk::init_superblock()
{
    strcpy(superblock.name, AUTHOR);
    strcpy(superblock.name, DESC);

    superblock.fat_addr_start = sizeof(superblock);
    superblock.bitmap_addr_start = superblock.fat_addr_start + sizeof(fatTable);
    superblock.data_addr_start = superblock.bitmap_addr_start + sizeof(bitMap);
}

void Disk::init_fatTable()
{
    memset(fatTable, 0, sizeof(fatTable));
}

void Disk::init_bitMap()
{
    memset(bitMap, CLUSTER_FREE, sizeof(bitMap));
}

Disk::dir_t *Disk::init_dir(uint32_t start_cluster, uint32_t parent_start_cluster)
{
    dir_t *dir = (dir_t *)malloc(sizeof(dir_t));

    if (dir == NULL)
        LOG(Log::ERROR, "directory is null");

    dir->parent_start_cluster = 0;
    dir->entries_count = 2;

    dir->entries = (dir_entry_t *)malloc(sizeof(dir_entry_t) * dir->entries_count);

    strcpy(dir->entries[0].name, ".");
    dir->entries[0].size = sizeof(dir->entries[0]); /*get_directory_size(*root);*/
    dir->entries[0].start_cluster = start_cluster;
    dir->entries[0].is_directory = 1;

    strcpy(dir->entries[1].name, "..");
    dir->entries[1].size = sizeof(dir->entries[1]); /*get_directory_size(*root);*/
    dir->entries[1].start_cluster = parent_start_cluster;
    dir->entries[1].is_directory = 1;
    return dir;
}

void Disk::createDisk()
{
    size_t size = sizeof(superblock_t) + sizeof(fatTable) + sizeof(bitMap) + CLUSTER_SIZE * CLUSTER_COUNT;

    disk = fopen(FILE_NAME, "wb");

    ftruncate(fileno(disk), size);
    rewind(disk);

    fclose(disk);
    disk = fopen(FILE_NAME, "rb+");
}

void Disk::save_superblock()
{
    fseek(disk, SUPERBLOCK_ADDR_START, SEEK_SET);
    fwrite(&superblock, sizeof(superblock), 1, disk);
    fflush(disk);
}

void Disk::save_fat_table()
{
    fseek(disk, superblock.fat_addr_start, SEEK_SET);
    fwrite(&fatTable, sizeof(fatTable), 1, disk);
    fflush(disk);
}

void Disk::save_bit_map()
{
    fseek(disk, superblock.bitmap_addr_start, SEEK_SET);
    fwrite(&bitMap, sizeof(bitMap), 1, disk);
    fflush(disk);
}

uint8_t Disk::save_dir(dir_t *&dir)
{
    int number_of_entries_in_first_cluster = (CLUSTER_SIZE - sizeof(uint32_t) - sizeof(uint32_t)) / sizeof(dir_entry_t);
    int number_of_remaining_entries = dir->entries_count - number_of_entries_in_first_cluster;

    if (n_free_clusters(1) == 0 || CLUSTER_SIZE < sizeof(dir))
    {
        LOG(Log::WARNING, "Not enough clusters to create and store directory");
        return 0;
    }

    uint32_t first_cluster_index = attain_cluster();

    dir->entries[0].start_cluster = first_cluster_index;

    fseek(disk, superblock.data_addr_start + first_cluster_index * CLUSTER_SIZE, SEEK_SET);
    fwrite(&dir->parent_start_cluster, sizeof(uint32_t), 1, disk);
    fflush(disk);

    fseek(disk, superblock.data_addr_start + first_cluster_index * CLUSTER_SIZE + sizeof(uint32_t), SEEK_SET);
    fwrite(&dir->entries_count, sizeof(uint32_t), 1, disk);
    fflush(disk);

    for (int i = 0; i < min(dir->entries_count, number_of_entries_in_first_cluster); i++)
    {
        long int addr = (superblock.data_addr_start +
                         (first_cluster_index * CLUSTER_SIZE) + sizeof(uint32_t) + sizeof(uint32_t));

        fseek(disk, addr + i * sizeof(dir_entry_t), SEEK_SET);
        fwrite(&dir->entries[i], sizeof(dir_entry_t), 1, disk);
        fflush(disk);
    }

    if (number_of_remaining_entries <= 0)
    {
        fatTable[first_cluster_index] = CLUSTER_EOF;
        return 1;
    }

    int number_of_entries_per_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);
    int number_of_clusters_needed = number_of_remaining_entries / number_of_entries_per_cluster;

    if (number_of_entries_per_cluster % number_of_clusters_needed > 0)
        number_of_clusters_needed++;

    if (n_free_clusters(number_of_clusters_needed) == 0)
    {
        bitMap[first_cluster_index] = CLUSTER_FREE;
        return 0;
    }

    uint32_t *clusters = (uint32_t *)malloc(sizeof(uint32_t) * number_of_clusters_needed);

    for (int i = 0; i < number_of_clusters_needed; i++)
    {
        clusters[i] = attain_cluster();
    }

    uint32_t index = number_of_entries_in_first_cluster;

    for (int i = 0; i < number_of_clusters_needed - 1; i++)
    {
        fseek(disk, superblock.data_addr_start + clusters[i] * CLUSTER_SIZE, SEEK_SET);
        fwrite(&dir->entries[index], sizeof(dir_entry_t), number_of_entries_per_cluster, disk);
        fflush(disk);
        number_of_remaining_entries -= number_of_entries_per_cluster;
        index += number_of_entries_per_cluster;
    }

    fseek(disk, superblock.data_addr_start + clusters[number_of_clusters_needed - 1] * CLUSTER_SIZE, SEEK_SET);
    fwrite(&dir->entries[index], sizeof(dir_entry_t), number_of_remaining_entries, disk);
    fflush(disk);

    fatTable[first_cluster_index] = clusters[0];
    for (int i = 1; i < number_of_clusters_needed; i++)
    {
        fatTable[clusters[i - 1]] = clusters[i];
    }

    fatTable[clusters[number_of_clusters_needed - 1]] = CLUSTER_EOF;

    free(clusters);
    save_bit_map();
    save_fat_table();
    return 1;
}

void Disk::load_file_system()
{
    LOG(Log::INFO, "loading the file system from the disk");

    LOG(Log::INFO, "opening the disk...");
    disk = fopen(FILE_NAME, "rb+");

    load_super_block();
    load_fat_table();
    load_bitmap();
    LOG(Log::INFO, "loading the root directory...");
    root = read_dir(ROOT_START_CLUSTER);
}

void Disk::load_super_block()
{
    fseek(disk, 0, SEEK_SET);
    fread(&superblock, sizeof(superblock_t), 1, disk);
    LOG(Log::INFO, "loading the superblock...");
}

void Disk::load_fat_table()
{
    fseek(disk, superblock.fat_addr_start, SEEK_SET);
    fread(&fatTable, sizeof(fatTable), 1, disk);
    LOG(Log::INFO, "loading the fat table...");
}

void Disk::load_bitmap()
{
    fseek(disk, superblock.bitmap_addr_start, SEEK_SET);
    fread(&bitMap, sizeof(bitMap), 1, disk);
    LOG(Log::INFO, "loading the bit map...");
}

Disk::dir_t *Disk::read_dir(uint32_t start_cluster_index)
{
    dir_t *tmp = (dir_t *)malloc(sizeof(dir_t));

    if (tmp == NULL)
    {
        free(tmp);
        return NULL;
    }

    fseek(disk, superblock.data_addr_start + start_cluster_index * CLUSTER_SIZE, SEEK_SET);
    fread(&tmp->parent_start_cluster, sizeof(uint32_t), 1, disk);

    fseek(disk, superblock.data_addr_start + start_cluster_index * CLUSTER_SIZE + sizeof(uint32_t), SEEK_SET);
    fread(&tmp->entries_count, sizeof(uint32_t), 1, disk);

    tmp->entries = (dir_entry_t *)malloc(sizeof(dir_entry_t) * tmp->entries_count);

    if (tmp->entries == NULL)
    {
        free(tmp->entries);
        return NULL;
    }

    int number_of_entries_in_first_cluster = (CLUSTER_SIZE - sizeof(uint32_t) - sizeof(uint32_t)) / sizeof(dir_entry_t);
    uint32_t read_directory_entries = 0;

    for (int i = 0; i < min(number_of_entries_in_first_cluster, tmp->entries_count); i++)
    {
        long int addr = (superblock.data_addr_start + start_cluster_index * CLUSTER_SIZE) + sizeof(uint32_t) + sizeof(uint32_t);

        fseek(disk, addr + i * sizeof(dir_entry_t), SEEK_SET);
        fread(&tmp->entries[read_directory_entries++], sizeof(dir_entry_t), 1, disk);
    }

    start_cluster_index = fatTable[start_cluster_index];

    if (tmp->entries_count <= number_of_entries_in_first_cluster)
        return tmp;

    int number_of_entries_per_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);

    while (fatTable[start_cluster_index] != CLUSTER_EOF)
    {

        fseek(disk, superblock.data_addr_start + start_cluster_index * CLUSTER_SIZE, SEEK_SET);
        fread(&tmp->entries[read_directory_entries], sizeof(dir_entry_t), number_of_entries_per_cluster, disk);
        start_cluster_index = fatTable[start_cluster_index];
        read_directory_entries += number_of_entries_per_cluster;
    }

    fseek(disk, superblock.data_addr_start + start_cluster_index * CLUSTER_SIZE, SEEK_SET);
    fread(&tmp->entries[read_directory_entries], sizeof(dir_entry_t), (tmp->entries_count - read_directory_entries), disk);
    return tmp;
}

uint8_t Disk::insert_file(const char *name, FILE *file)
{
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    int number_of_clusters_needed = file_size / CLUSTER_SIZE;

    if (file_size % CLUSTER_SIZE != 0)
        number_of_clusters_needed++;

    if (n_free_clusters(number_of_clusters_needed) == 0)
    {
        LOG(Log::WARNING, "number of clusters to store file is not available");
        return 0;
    }

    uint32_t *clusters = (uint32_t *)malloc(sizeof(uint32_t) * number_of_clusters_needed);

    for (int i = 0; i < number_of_clusters_needed; i++)
        clusters[i] = attain_cluster();

    char *buffer = (char *)malloc(CLUSTER_SIZE);

    for (int i = 0; i < number_of_clusters_needed - 1; i++)
    {
        fseek(disk, superblock.data_addr_start + clusters[i] * CLUSTER_SIZE, SEEK_SET);
        fseek(file, i * CLUSTER_SIZE, SEEK_SET);
        fread(buffer, sizeof(char), CLUSTER_SIZE, file);
        fwrite(buffer, sizeof(char), CLUSTER_SIZE, disk);
    }

    fseek(disk, superblock.data_addr_start + clusters[number_of_clusters_needed - 1] * CLUSTER_SIZE, SEEK_SET);
    fseek(file, (file_size / CLUSTER_SIZE) * CLUSTER_SIZE, SEEK_SET);
    fread(buffer, sizeof(char), file_size % CLUSTER_SIZE, file);
    fwrite(buffer, sizeof(char), file_size % CLUSTER_SIZE, disk);

    for (int i = 1; i < number_of_clusters_needed; i++)
        fatTable[clusters[i - 1]] = clusters[i];

    fatTable[clusters[number_of_clusters_needed - 1]] = CLUSTER_EOF;

    dir_entry_t *new_file = (dir_entry_t *)malloc(sizeof(dir_entry_t));

    strcpy(new_file->name, name);
    new_file->is_directory = 0;
    new_file->size = file_size;
    new_file->start_cluster = clusters[0];

    //TODO FIX, same as dir
    extend_dir(root, new_file, 0);

    free(clusters);
    free(buffer);
    return 1;
}
uint8_t Disk::insert_dir(const char *name, dir_t *&current_working_directory, uint32_t dir_start_cluster)
{
    dir_t *dir = init_dir(0, dir_start_cluster);

    if (save_dir(dir) == 0)
    {
        LOG(Log::WARNING, "not enough clusters to create directory");
        free(dir);
        return 0;
    }

    dir_entry_t *new_dir = (dir_entry_t *)malloc(sizeof(dir_entry_t));

    strcpy(new_dir->name, name);
    new_dir->is_directory = 1;
    new_dir->size = directory_size(dir);
    new_dir->start_cluster = dir->entries[0].start_cluster;

    free(dir);
    extend_dir(current_working_directory, new_dir, dir_start_cluster);
    return 1;
}

void Disk::extend_dir(dir_t *dir, dir_entry_t *entry, uint32_t curr_dir_start_cluster)
{
    if (dir == NULL || entry == NULL)
        return;

    dir_entry_t *extended_dir = (dir_entry_t *)malloc(sizeof(dir_entry_t) * (dir->entries_count + 1));

    for (int i = 0; i < dir->entries_count; i++)
        extended_dir[i] = dir->entries[i];

    extended_dir[dir->entries_count] = *entry;

    free(dir->entries);
    dir->entries_count++;
    dir->entries = extended_dir;

    uint32_t dir_start_index = curr_dir_start_cluster;
    uint32_t tmp;

    while (dir_start_index != CLUSTER_EOF)
    {
        bitMap[dir_start_index] = CLUSTER_FREE;
        tmp = dir_start_index;
        dir_start_index = fatTable[dir_start_index];
        fatTable[tmp] = CLUSTER_FREE;
    }

    save_dir(dir);
    save_bit_map();
    save_fat_table();
}

uint8_t Disk::n_free_clusters(int i)
{
    int counter = 0;
    for (int i = 0; i < CLUSTER_COUNT; i++)
    {
        if (bitMap[i] == CLUSTER_FREE)
            counter++;
    }

    return (counter >= i) ? 1 : 0;
}

uint32_t Disk::attain_cluster()
{
    for (int i = 0; i < CLUSTER_COUNT; i++)
    {
        if (bitMap[i] == CLUSTER_FREE)
        {
            bitMap[i] = CLUSTER_BUSY;
            return i;
        }
    }
    return 0;
    LOG(Log::WARNING, "a free cluster was not able to be found");
}

uint32_t Disk::find_entry(dir_t *dir, const char *name, uint8_t flag)
{
    for (int i = 0; i < dir->entries_count; i++)
    {
        if (!(strcmp(dir->entries[i].name, name)))
        {
            if (flag == 1 && dir->entries[i].is_directory == 1)
                return dir->entries[i].start_cluster;
            if (flag == 0 && dir->entries->is_directory == 0)
                return dir->entries[i].start_cluster;
        }
    }

    return -1;
}

void Disk::print_dir(dir_t &dir)
{
    printf("Start cluster index > %    d\n", dir.parent_start_cluster);
    printf("Entries count > %    d\n", dir.entries_count);

    printf("ID %5s Size %4s Name\n-----------------------\n", " ", " ");
    for (int i = 0; i < dir.entries_count; i++)
    {
        printf("[%d] %4s %dB %4s %s\n", i, " ", dir.entries[i].size, " ", dir.entries[i].name);
    }
}

void Disk::print_fat_table()
{
    printf("--------------\n");
    printf("  Fat Table\n");
    printf("--------------\n");

    for (int i = 0; i < CLUSTER_COUNT; i++)
    {
        printf("  %.2d   %.2d\n", i, fatTable[i]);
    }
}

void Disk::print_bit_map()
{
    printf("--------------\n");
    printf("  bit Map\n");
    printf("--------------\n");

    for (int i = 0; i < CLUSTER_COUNT; i++)
    {
        printf("  %.2d   %.2d\n", i, bitMap[i]);
    }
}

void Disk::print_file(uint32_t start_cluster, uint32_t size)
{

    char *buffer = (char *)malloc(CLUSTER_SIZE);

    while (fatTable[start_cluster] != CLUSTER_EOF)
    {
        fseek(disk, superblock.data_addr_start + start_cluster * CLUSTER_SIZE, SEEK_SET);
        fread(buffer, sizeof(char), CLUSTER_SIZE, disk);
        printf("%s", buffer);
        start_cluster = fatTable[start_cluster];
    }

    uint32_t remainder = size % CLUSTER_SIZE;
    fseek(disk, superblock.data_addr_start + start_cluster * CLUSTER_SIZE, SEEK_SET);
    fread(&buffer, sizeof(char), remainder, disk);
    buffer[remainder] = '\0';
    printf("%s", buffer);

    free(buffer);
}

size_t Disk::directory_size(dir_t *&dir)
{
    size_t size = sizeof(uint32_t) + sizeof(uint32_t);

    for (int i = 0; i < dir->entries_count; i++)
        size += sizeof(dir_entry_t);

    return size;
}