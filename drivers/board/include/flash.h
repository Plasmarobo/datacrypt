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
#define FLASH_ERR_UNKNOWN_ID (-9)

#define PAGE_SIZE (2048)
#define BLOCK_COUNT (1024)
#define PAGES_PER_BLOCK (64)
#define OOB_SIZE (64)

#define PAGE_MASK (0x3F)
#define BLOCK_MASK (0xFFC0)
#define BLOCK(x) ((x & BLOCK_MASK) >> 6)
#define PAGE(x) (x & PAGE_MASK)

#define ERASED_VALUE (0xFF)
#define BAD_BLOCK_VALUE (0x18)
#define PAGE_ADDRESS(block, page) \
    (((block << 6) & BLOCK_MASK) | (page & PAGE_MASK))
#define PAGE_OFFSET(offset) (offset / PAGE_SIZE)

#define OOB_BASE_ADDRESS (PAGE_SIZE)

typedef enum {
    FLASH_EV_IDLE,
    FLASH_EV_BAD_BLOCK,
    FLASH_EV_TIMEOUT,
    FLASH_EV_MAX,
} flash_event_t;

typedef uint16_t flash_page_address_t;

void flash_init(callback_t on_init);
void flash_read(flash_page_address_t bp_addr, uint16_t byte_address_,
                buffer_t dest, length_t size, callback_t on_complete);
void flash_write(flash_page_address_t page, uint16_t byte_address_,
                 buffer_t data, length_t size, callback_t on_complete);
void flash_update(flash_page_address_t page, uint16_t byte_address_,
                  buffer_t data, length_t size, callback_t on_complete);
void flash_commit(callback_t on_complete);
void flash_erase(uint32_t addr, callback_t on_complete);
void flash_register_event_handler(flash_event_t event, callback_t handler);

#endif // FLASH_H
