#ifndef FLASH_H
#define FLASH_H

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

#endif // FLASH_H