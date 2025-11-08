#ifndef FLASH_H
#define FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_SUCCESS (0)
#define FLASH_ERR_TIMEOUT (-1)
#define FLASH_ERR_BUSY (-2)
// A write was not properly setup
#define FLASH_ERR_NOT_SETUP (-3)
#define FLASH_ERR_BAD_BLOCK (-4)
#define FLASH_ERR_BAD_STATE (-5)
#define FLASH_ERR_INVALID_ARG (-6)
#define FLASH_ERR_FAILURE (-7)
#define FLASH_ERR_CACHE_OVERWRITE (-8)

#define PAGE_SIZE (2048)
#define BLOCK_COUNT (1024)
#define PAGES_PER_BLOCK (64)

#define PAGE_MASK (0x1F)
#define BLOCK_MASK (0xFFE0)
#define BLOCK(x) ((x & BLOCK_MASK) >> 5)
#define PAGE(x) (x & PAGE_MASK)

#define ERASED_VALUE (0xFF)
#define BAD_BLOCK_VALUE (0x18)
#define PAGE_ADDRESS(block, page) \
    (((block << 5) & BLOCK_MASK) | (page & PAGE_MASK))
#define PAGE_OFFSET(offset) (offset / PAGE_SIZE)

#define OOB_BASE_ADDRESS (PAGE_SIZE)

#ifdef __cplusplus
}
#endif

#endif // FLASH_H
