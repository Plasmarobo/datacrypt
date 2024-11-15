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

// Upper 11 bits
#define PAGE_MASK (0x3F)
#define BLOCK_MASK (0xFFC0)
// Lower 11 bits
#define BYTE_MASK (0x7FF)

#define BLOCK(addr) (addr >> 17)
#define PAGE(addr) ((addr >> 11) & PAGE_MASK)
#define BLOCK_PAGE(addr) (FLASH_PAGE_ADDR(BLOCK(addr), PAGE(addr)))
#define BYTE(addr) (addr & BYTE_MASK)

#define FLASH_PAGE_ADDR(block, page) (((block << 6) & BLOCK_MASK) | (page & PAGE_MASK))
#define FLASH_BYTE_OFFSET(offset) (offset & BYTE_MASK)
#define FLASH_ADDRESS(block, page, offset) ((FLASH_PAGE_ADDR(block, page) << 11) | FLASH_BYTE_OFFSET(offset))

#define ERASED_VALUE (0xFF)
#define BAD_BLOCK_VALUE (0x18)

#define OOB_BASE_ADDRESS (PAGE_SIZE)

#define BLOCK_SIZE (2048 * 64)
#define PAGE_SIZE (2048)
#define BLOCK_COUNT (1024)
#define PAGES_PER_BLOCK (64)
#define OOB_SIZE (64)

typedef enum {
    FLASH_EV_IDLE,
    FLASH_EV_BAD_BLOCK,
    FLASH_EV_TIMEOUT,
    FLASH_EV_MAX,
} flash_event_t;

typedef uint32_t flash_address_t;

void flash_init(callback_t on_init);
void flash_read(flash_address_t address,
                buffer_t dest, length_t size, callback_t on_complete);
void flash_write(flash_address_t address,
                 buffer_t data, length_t size, callback_t on_complete);
void flash_update(flash_address_t address,
                  buffer_t data, length_t size, callback_t on_complete);
void flash_commit(callback_t on_complete);
void flash_erase(uint32_t addr, callback_t on_complete);
void flash_register_event_handler(flash_event_t event, callback_t handler);

#endif // FLASH_H
