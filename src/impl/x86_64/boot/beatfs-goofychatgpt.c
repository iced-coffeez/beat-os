#include "beatfs-goofychatgpt.h"
#include "io.h"     // must provide inb/outb/inw/outw/outl/outl etc.
#include "print.h"
#include <stdint.h>
#include <stddef.h>

/* Implementation notes:
 * - Superblock is stored at LBA 1 on the selected ATA drive.
 * - File data starts at LBA 2, files are stored contiguously.
 * - All calls are synchronous PIO; simple timeouts are used to avoid infinite hangs.
 */

/* ---- internal state ---- */
static BeatFS_Superblock sb;
static uint8_t ata_drive_selected = ATA_DRIVE_MASTER; // 0 = master, 1 = slave
static uint8_t tmp_sector[BEATFS_SECTOR_SZ];

/* ---- small helpers (freestanding-friendly) ---- */
static void str_copy(char *dst, const char *src, size_t max) {
    size_t i = 0;
    if (max == 0) return;
    while (i < max - 1 && src[i] != '\0') { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return (*a == *b);
}

static void print_u32(uint32_t v) {
    char buf[12]; int i = 0;
    if (v == 0) { print_char('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    for (int j = i - 1; j >= 0; --j) print_char(buf[j]);
}

/* ---- I/O timing helper ---- */
static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

/* ---- ATA helpers (primary channel) ----
 * Primary command port base = 0x1F0
 * Status register = base + 7
 * Drive/head = base + 6
 */
#define ATA_IO_BASE 0x1F0
#define ATA_STATUS_PORT (ATA_IO_BASE + 7)
#define ATA_DATA_PORT   (ATA_IO_BASE + 0) /* inw/outw at 0x1F0 */

static bool ata_poll_bsy_clear(uint32_t timeout_loops) {
    while (timeout_loops--) {
        uint8_t s = inb(ATA_STATUS_PORT);
        if (!(s & 0x80)) return true; // BSY cleared
    }
    return false;
}

static bool ata_wait_drq_or_err(uint32_t timeout_loops) {
    while (timeout_loops--) {
        uint8_t s = inb(ATA_STATUS_PORT);
        if (s & 0x08) return true; /* DRQ */
        if (s & 0x01) return false; /* ERR */
    }
    return false;
}

/* select drive/head and (for LBA48 you'd do more) set LBA high bits when needed */
static void ata_select_drive(uint8_t drive, uint32_t lba_high_nibble) {
    uint8_t val = 0xE0 | ((drive & 1) << 4) | (lba_high_nibble & 0x0F);
    outb(ATA_IO_BASE + 6, val);
    io_wait();
}

/* IDENTIFY */
bool identify_drive(uint8_t drive, uint16_t *out_buffer) {
    /* select drive (A0/A1 style) and send IDENTIFY (0xEC) */
    uint8_t select = 0xA0 | ((drive & 1) << 4);
    outb(ATA_IO_BASE + 6, select);
    io_wait();

    outb(ATA_IO_BASE + 7, 0xEC); /* IDENTIFY */
    io_wait();

    /* wait for BSY clear */
    if (!ata_poll_bsy_clear(1000000)) return false;

    uint8_t status = inb(ATA_IO_BASE + 7);
    if (status == 0) return false; /* no device */

    /* If ERR bit set, fail */
    if (status & 0x01) return false;

    /* Wait for DRQ */
    if (!ata_wait_drq_or_err(1000000)) return false;

    /* Read 256 words */
    for (int i = 0; i < 256; ++i) {
        outb(0,0); /* avoid strict alias optimization issues, do nothing - keep io_wait pattern? */
        uint16_t w = inw(ATA_DATA_PORT);
        if (out_buffer) out_buffer[i] = w;
    }
    return true;
}

/* Read single sector (LBA28) */
bool ata_read_sector(uint8_t drive, uint32_t lba, void *out_sector) {
    /* prepare LBA28 */
    ata_select_drive(drive, (lba >> 24) & 0x0F);
    outb(ATA_IO_BASE + 2, 1); /* sector count = 1 */
    outb(ATA_IO_BASE + 3, (uint8_t)(lba & 0xFF));        /* LBA low */
    outb(ATA_IO_BASE + 4, (uint8_t)((lba >> 8) & 0xFF)); /* LBA mid */
    outb(ATA_IO_BASE + 5, (uint8_t)((lba >> 16) & 0xFF));/* LBA high */
    outb(ATA_IO_BASE + 7, 0x20); /* READ SECTORS */
    io_wait();

    if (!ata_poll_bsy_clear(1000000)) return false;
    if (!ata_wait_drq_or_err(1000000)) return false;

    /* read 256 words */
    uint16_t *dst = (uint16_t*) out_sector;
    for (int i = 0; i < 256; ++i) {
        dst[i] = inw(ATA_DATA_PORT);
    }
    return true;
}

/* Write single sector (LBA28) */
bool ata_write_sector(uint8_t drive, uint32_t lba, const void *sector) {
    ata_select_drive(drive, (lba >> 24) & 0x0F);
    outb(ATA_IO_BASE + 2, 1); /* sector count */
    outb(ATA_IO_BASE + 3, (uint8_t)(lba & 0xFF));
    outb(ATA_IO_BASE + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_IO_BASE + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_IO_BASE + 7, 0x30); /* WRITE SECTORS */
    io_wait();

    if (!ata_poll_bsy_clear(1000000)) return false;
    if (!ata_wait_drq_or_err(1000000)) return false;

    const uint16_t *src = (const uint16_t*) sector;
    for (int i = 0; i < 256; ++i) {
        outw(ATA_DATA_PORT, src[i]);
        io_wait(); /* tiny delay to let controller accept words */
    }

    /* flush cache */
    outb(ATA_IO_BASE + 7, 0xE7);
    io_wait();
    (void)ata_poll_bsy_clear(1000000);

    return true;
}

/* Write multiple contiguous sectors */
static bool ata_write_sectors(uint8_t drive, uint32_t lba, const void *buf, uint32_t count) {
    const uint8_t *ptr = (const uint8_t*) buf;
    for (uint32_t i = 0; i < count; ++i) {
        if (!ata_write_sector(drive, lba + i, ptr + i * BEATFS_SECTOR_SZ)) return false;
    }
    return true;
}

/* Read multiple contiguous sectors */
static bool ata_read_sectors(uint8_t drive, uint32_t lba, void *buf, uint32_t count) {
    uint8_t *ptr = (uint8_t*) buf;
    for (uint32_t i = 0; i < count; ++i) {
        if (!ata_read_sector(drive, lba + i, ptr + i * BEATFS_SECTOR_SZ)) return false;
    }
    return true;
}

/* ---- beatfs superblock on-disk layout:
 * store sb at LBA 1 (sector 1). writing/reading uses ata_read_sector/ata_write_sector.
 */

/* load superblock from disk (if present), else init empty fs */
void beatfs_init(uint8_t ata_drive) {
    ata_drive_selected = ata_drive & 1;

    /* attempt read LBA 1 */
    uint8_t tmp[BEATFS_SECTOR_SZ];
    if (ata_read_sector(ata_drive_selected, 1, tmp)) {
        /* if magic matches, load, else initialize new */
        BeatFS_Superblock *x = (BeatFS_Superblock*) tmp;
        if (x->magic == BEATFS_MAGIC && x->version == BEATFS_VERSION) {
            /* copy into memory sb */
            sb = *x;
            print("beat!fs: superblock loaded from disk.\n");
            return;
        }
    }

    /* otherwise initialize new in-memory superblock and write it */
    sb.magic = BEATFS_MAGIC;
    sb.version = BEATFS_VERSION;
    sb.file_count = 0;
    for (size_t i = 0; i < BEATFS_MAX_FILES; ++i) {
        sb.files[i].name[0] = '\0';
        sb.files[i].size = 0;
        sb.files[i].start_lba = 0;
        sb.files[i].blocks = 0;
    }

    /* write to disk */
    for (size_t i = 0; i < BEATFS_SECTOR_SZ; ++i) tmp[i] = 0;
    /* copy sb into tmp */
    size_t copy = sizeof(BeatFS_Superblock);
    if (copy > BEATFS_SECTOR_SZ) copy = BEATFS_SECTOR_SZ;
    /* simple byte copy */
    for (size_t i = 0; i < copy; ++i) ((uint8_t*)&tmp[0])[i] = ((uint8_t*)&sb)[i];
    ata_write_sector(ata_drive_selected, 1, tmp);
    print("beat!fs: new superblock written.\n");
}

/* flush in-memory superblock to LBA 1 */
static void beatfs_flush(void) {
    uint8_t tmp[BEATFS_SECTOR_SZ];
    for (size_t i = 0; i < BEATFS_SECTOR_SZ; ++i) tmp[i] = 0;
    size_t copy = sizeof(BeatFS_Superblock);
    if (copy > BEATFS_SECTOR_SZ) copy = BEATFS_SECTOR_SZ;
    for (size_t i = 0; i < copy; ++i) ((uint8_t*)&tmp[0])[i] = ((uint8_t*)&sb)[i];
    ata_write_sector(ata_drive_selected, 1, tmp);
}

/* create file metadata (alloc contiguous sectors) */
int beatfs_create(const char *name) {
    if (sb.file_count >= BEATFS_MAX_FILES) { 
        print("beat!fs: no metadata slots\n"); 
        return -1; 
    }

    BeatFS_File *f = &sb.files[sb.file_count];
    str_copy(f->name, name, BEATFS_MAX_NAME);
    f->size = 0;
    f->start_lba = 0;  // allocated lazily
    f->blocks = 0;

    sb.file_count++;
    beatfs_flush();

    print("beat!fs: created empty file \"");
    print(f->name);
    print("\"\n");

    return 0;
}


int beatfs_write_file(const char *name, const void *buffer, uint32_t size) {
    for (uint32_t i = 0; i < sb.file_count; ++i) {
        BeatFS_File *f = &sb.files[i];
        if (str_eq(f->name, name)) {
            uint32_t needed_blocks = (size + BEATFS_SECTOR_SZ - 1) / BEATFS_SECTOR_SZ;

            /* allocate if needed */
            if (needed_blocks > f->blocks) {
                uint32_t base = 2;
                for (uint32_t j = 0; j < sb.file_count; ++j) {
                    uint32_t end = sb.files[j].start_lba + sb.files[j].blocks;
                    if (end > base) base = end;
                }

                f->start_lba = base;
                f->blocks = needed_blocks;
            }

            if (!ata_write_sectors(ata_drive_selected, f->start_lba, buffer, f->blocks)) {
                print("beat!fs: write failed\n");
                return -1;
            }

            f->size = size; // update size to actual bytes written
            beatfs_flush();

            print("beat!fs: wrote ");
            print_u32(size);
            print(" bytes to \"");
            print(f->name);
            print("\"\n");

            return 0;
        }
    }
    print("beat!fs: file not found\n");
    return -1;
}


/* read file contents (max_size bytes) */
int beatfs_read_file(const char *name, void *out, uint32_t max_size) {
    for (uint32_t i = 0; i < sb.file_count; ++i) {
        BeatFS_File *f = &sb.files[i];
        if (str_eq(f->name, name)) {
            uint32_t to_read = f->size;
            if (to_read > max_size) to_read = max_size;
            uint32_t blocks = (to_read + BEATFS_SECTOR_SZ - 1) / BEATFS_SECTOR_SZ;
            if (!ata_read_sectors(ata_drive_selected, f->start_lba, out, blocks)) {
                print("beat!fs: read failed\n"); return -1;
            }
            return (int) to_read;
        }
    }
    print("beat!fs: file not found\n");
    return -1;
}

void beatfs_list(void) {
    print("beat!fs files:\n");
    for (uint32_t i = 0; i < sb.file_count; ++i) {
        BeatFS_File *f = &sb.files[i];
        print(" - ");
        print(f->name);
        print(": ");
        print_u32(f->size);
        print(" bytes (allocated ");
        print_u32(f->blocks * BEATFS_SECTOR_SZ);
        print(" bytes, LBA ");
        print_u32(f->start_lba);
        print(")\n");
    }
    if (sb.file_count == 0) {
        print("  <empty>\n");
    }
}


int beatfs_remove(const char *name) {
    for (uint32_t i = 0; i < sb.file_count; ++i) {
        if (str_eq(sb.files[i].name, name)) {
            // Shift later entries down
            for (uint32_t j = i; j < sb.file_count - 1; ++j) {
                sb.files[j] = sb.files[j + 1];
            }

            sb.file_count--;

            // Clear last slot
            sb.files[sb.file_count].name[0] = '\0';
            sb.files[sb.file_count].size = 0;
            sb.files[sb.file_count].start_lba = 0;
            sb.files[sb.file_count].blocks = 0;

            beatfs_flush();
            print("beat!fs: removed ");
            print(name);
            print("\n");
            return 0;
        }
    }
    print("beat!fs: file not found\n");
    return -1;
}
