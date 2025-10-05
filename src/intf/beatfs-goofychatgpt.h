#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BEATFS_MAGIC      0xB347F5u
#define BEATFS_VERSION    1u
#define BEATFS_MAX_FILES  16
#define BEATFS_MAX_NAME   32
#define BEATFS_SECTOR_SZ  512u

// ATA drives (pass 0 for master, 1 for slave)
#define ATA_DRIVE_MASTER 0
#define ATA_DRIVE_SLAVE  1

typedef struct {
    char     name[BEATFS_MAX_NAME];
    uint32_t size;        // bytes
    uint32_t start_lba;   // LBA of first sector
    uint32_t blocks;      // sectors allocated
} BeatFS_File;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t file_count;
    BeatFS_File files[BEATFS_MAX_FILES];
} BeatFS_Superblock;

/* API */
void        beatfs_init(uint8_t ata_drive); /* init superblock (in-memory) and set drive */
int         beatfs_create(const char *name); /* alloc metadata and flush */
int         beatfs_write_file(const char *name, const void *buffer, uint32_t size); /* write file data */
int         beatfs_read_file(const char *name, void *out, uint32_t max_size); /* read data */
int         beatfs_remove(const char *name);
void        beatfs_list(void);

/* Low-level helpers you can use directly */
bool        identify_drive(uint8_t drive, uint16_t *out_buffer); /* returns whether drive present */
bool        ata_read_sector(uint8_t drive, uint32_t lba, void *out_sector); /* reads one 512B sector */
bool        ata_write_sector(uint8_t drive, uint32_t lba, const void *sector); /* writes one 512B sector */
