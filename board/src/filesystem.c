#include "filesystem.h"

#include "lfs.h"
#include "lfs_util.h"

#include "bsp.h"
#include "flash.h"

#define LFS_BUFFER_LEN (32)
#define LFS_LOOKAHEAD (32)
#define FILESYSTEM_TIMEOUT_MS (500)

static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size);
static int lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);
static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block);
static int lfs_flash_sync(const struct lfs_config *c);

static uint8_t lfs_read_buffer[LFS_BUFFER_LEN];
static uint8_t lfs_write_buffer[LFS_BUFFER_LEN];
static uint8_t lfs_cache_buffer[LFS_BUFFER_LEN];
static uint8_t lfs_lookahead_buffer[LFS_BUFFER_LEN];

static struct lfs_config lfs_cfg = {

    // Opaque user provided context that can be used to pass
    // information to the block device operations
    .context = NULL,
    // Read a region in a block. Negative error codes are propagated
    // to the user.
    .read = lfs_flash_read,
    .prog = lfs_flash_prog,
    .erase = lfs_flash_erase,
    .sync = lfs_flash_sync,

    // Align to 32 byte read/write 
    .read_size = LFS_BUFFER_LEN, // Our chip supports up to 1 byte reads, but loads a page into cache
    .prog_size = PAGE_SIZE,
    .block_size = PAGES_PER_BLOCK * PAGE_SIZE,
    .block_count = BLOCK_COUNT,
    .block_cycles = 500, // arbitrary

    .cache_size = LFS_BUFFER_LEN,

    .lookahead_size = LFS_LOOKAHEAD,
    .compact_thresh = 0,
    .read_buffer = &lfs_read_buffer,
    .prog_buffer = &lfs_write_buffer,

    // Optional statically allocated lookahead buffer. Must be lookahead_size.
    // By default lfs_malloc is used to allocate this buffer.
    .lookahead_buffer = lfs_lookahead_buffer,

    // Optional upper limit on length of file names in bytes. No downside for
    // larger names except the size of the info struct which is controlled by
    // the LFS_NAME_MAX define. Defaults to LFS_NAME_MAX when zero. Stored in
    // superblock and must be respected by other littlefs drivers.
    .name_max = LFS_NAME_MAX,
    // Optional upper limit on files in bytes. No downside for larger files
    // but must be <= LFS_FILE_MAX. Defaults to LFS_FILE_MAX when zero. Stored
    // in superblock and must be respected by other littlefs drivers.
    .file_max = LFS_FILE_MAX,

    // Optional upper limit on custom attributes in bytes. No downside for
    // larger attributes size but must be <= LFS_ATTR_MAX. Defaults to
    // LFS_ATTR_MAX when zero.
    .attr_max = LFS_ATTR_MAX,

    // Optional upper limit on total space given to metadata pairs in bytes. On
    // devices with large blocks (e.g. 128kB) setting this to a low size (2-8kB)
    // can help bound the metadata compaction time. Must be <= block_size.
    // Defaults to block_size when zero.
    .metadata_max = 0,

    // Optional upper limit on inlined files in bytes. Inlined files live in
    // metadata and decrease storage requirements, but may be limited to
    // improve metadata-related performance. Must be <= cache_size, <=
    // attr_max, and <= block_size/8. Defaults to the largest possible
    // inline_max when zero.
    //
    // Set to -1 to disable inlined files.
    .inline_max = 0
};

static lfs_t lfs;
static lfs_file_t file;

static int get_lfs_err(int32_t status)
{
    int err = LFS_ERR_IO;
    switch(status)
    {
        case FLASH_ERR_BAD_BLOCK:
            err = LFS_ERR_CORRUPT;
            break;
        case FLASH_ERR_CACHE_OVERWRITE:
            err = LFS_ERR_FBIG;
            break;
        case FLASH_ERR_TIMEOUT:
        default:
            break;
    }
    return err;
}

static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{
    callback_t future = future_get();
    if (NULL != future)
    {
        uint16_t page = PAGE_OFFSET(off);
        flash_read(PAGE_ADDRESS(block, page), off % PAGE_SIZE, buffer, size, future);
        int32_t status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (FLASH_SUCCESS != status)
        {
            return get_lfs_err(status);
        }
    }
    return 0;
}

static int lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    callback_t future = future_get();
    if (NULL != future)
    {
        uint16_t page = PAGE_OFFSET(off);
        flash_write(PAGE_ADDRESS(block, page), off % PAGE_SIZE, buffer, size, future);
        int32_t status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (FLASH_SUCCESS != status)
        {
            return get_lfs_err(status);
        }
    }
    return 0;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    callback_t future = future_get();
    if (NULL != future)
    {
        flash_erase(PAGE_ADDRESS(block, 0), future);
        int32_t status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (FLASH_SUCCESS != status)
        {
            return get_lfs_err(status);
        }
    }
    return 0;
}

static int lfs_flash_sync(const struct lfs_config *c) {
    callback_t future = future_get();
    if (NULL != future)
    {
        flash_commit(future);
        int32_t status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (FLASH_SUCCESS != status)
        {
            return get_lfs_err(status);
        }
    }
    return 0;
}

void filesystem_init()
{
    int status = lfs_mount(&lfs, &lfs_cfg);
    if (status)
    {
        lfs_format(&lfs, &lfs_cfg);
        lfs_mount(&lfs, &lfs_cfg);
    }
}

void file_open(const char* path)
{
    lfs_file_opencfg(&lfs, &file, path, LFS_O_RDONLY, &lfs_cfg);
}

size_t file_read(buffer_t dest, size_t size)
{
    return lfs_file_read(&lfs, &file, dest, size);
}

void file_seek(int seekv)
{
    lfs_file_rewind(&lfs, &file);
    lfs_file_seek(&lfs, &file, seekv, LFS_SEEK_SET);
}

void file_close()
{
    lfs_file_close(&lfs, &file);
}