#include "bsp.h"
#include "fsm.h"
#include "ringbuffer.h"
#include "scheduler.h"
#include "stack.h"

#define CMD_SOFT_RESET (0xFF)
#define JEDEC_READ (0x9F)
// Can be used to poll registers/features, including busy
#define GET_FEATURE (0x0F)
// Can be used to set registers/features, such as WEL
#define SET_FEATURE (0x1F)
// Must be run before all program/erase operations
#define WRITE_ENABLE (0x06)
#define WRITE_DISABLE (0x04)
//
#define BLOCK_ERASE (0xD8)
// Loads data to be programmed into chip buffer, fills rest with 0xFF
#define PROGRAM_DATA_LOAD (0x02)
// Loads data to be programmed into chip buffer, does not alter rest
#define RANDOM_PROGRAM_DATA_LOAD (0x84)
#define QUAD_PROGRAM_DATA_LOAD (0x34)
// Commits program buffer to FLASH
#define PROGRAM_EXECUTE (0x10)

#define PAGE_READ_TO_CACHE (0x13)
#define READ_FROM_CACHE (0x03)
#define READ_FROM_CACHE_ALT (0x0B)
#define READ_FROM_CACHE_X2 (0x3B)
#define READ_FROM_CACHE_X4 (0x6B)

#define PAGE_SIZE (2048)
#define OOB (64)

#define BLOCK_COUNT (1024)
#define PAGES_PER_BLOCK (64)

#define FLASH_QUEUE_LENGTH (4)
#define FLASH_STATE_MAX_DEPTH (5)

// 32 bit header indicating the invalid page table follows
#define MAGIC_BYTES_INVALID_PAGE_TABLE (0x9A6E7ABE)
#define TRST_US (200)
#define TRD_US (25)
#define TPROG_US (700)
#define TERASE_MS (10)

typedef struct {
    // 1024 blocks of 64 pages
    uint64_t invalid_pages[BLOCK_COUNT];
} invalid_page_table_t;

typedef enum {
    FLASH_RESET,
    FLASH_READ_METADATA,
    FLASH_,
} flash_state_t;

typedef enum {
    FLASH_IDLE,
    FLASH_WRITE_COMMAND,
    FLASH_DUMMY_BYTES,
    FLASH_READ_BYTES,
} flash_transaction_state_t;

typedef struct {
    uint32_t base_addr;
    buffer_t data;
    length_t size;
    callback_t on_complete;
} flash_transaction_t;

RINGBUFFER(flash_queue, flash_transaction_t, FLASH_QUEUE_LENGTH);
STACK(flash_state, state_t*, FLASH_STATE_MAX_DEPTH);

static void next_state(void* ctx);

FSM(flash_fsm);
STATE_ENTER(reset) {
    stack_push(flash_state, idle);
    task_delayed(next_state, MICROS(TRST_US));
    // delay tRST
};
STATE_ENTER(get_feature){

};
STATE_ENTER(set_feature){

};
STATE_ENTER(poll_status){

};
STATE_UPDATE(poll_status){
    // Check the feature register
};
STATE_ENTER(populate_cache){
    // Kick off page read to cache
};
STATE_UPDATE(populate_cache){
    // Wait for tRD
};
STATE_ENTER(read_cache){
    // Stream to buffer
};
STATE_ENTER(block_erase){

};
STATE_ENTER(program_data){
    // Issues program data load
};
STATE_ENTER(program_exec){

};
STATE_ENTER(write_enable){

};
STATE_ENTER(read_iib){
    // Issue a read of page 0
};
STATE_UPDATE(read_iib){
    // Issue a read of page 1
};
STATE_ENTER(restore_iib){
    //
};
STATE_UPDATE(idle){
    // Flash is transactional, we don't need a task here
};
// Primary states
STATE_ENTER(write) {
    stack_push(poll_status);
    stack_push(program_exec);
    stack_push(write_enable);
    stack_push(program_data);
};
STATE_ENTER(read) {
    stack_push(poll_status);
    stack_push(read_cache);
    stack_push(populate_cache);
};
STATE_ENTER(erase) {
    stack_push(restore_iib);
    stack_push(poll_status);
    stack_push(block_erase);
    stack_push(write_enable);
    stack_push(read_iib);
};

REGISTER_STATE(flash_fsm, idle);
REGISTER_STATE(flash_fsm, reset);
REGISTER_STATE(flash_fsm, get_feature);
REGISTER_STATE(flash_fsm, set_feature);
REGISTER_STATE(flash_fsm, poll_status);
REGISTER_STATE(flash_fsm, populate_cache);
REGISTER_STATE(flash_fsm, read_cache);
REGISTER_STATE(flash_fsm, block_erase);
REGISTER_STATE(flash_fsm, program_data);
REGISTER_STATE(flash_fsm, program_exec);
REGISTER_STATE(flash_fsm, write_enable);
REGISTER_STATE(flash_fsm, write);
REGISTER_STATE(flash_fsm, read);
REGISTER_STATE(flash_fsm, erase);

static int32_t current_page = 0;
static int32_t current_block = 0;

static void next_state(void* ctx) {
    if (!stack_empty(flash_state)) {
        state_t* newstate;
        stack_pop(flash_state, &newstate);
        impl_fsm_set_state(flash_fsm, newstate);
    } else {
        // drop back to idle
        FSM_SET_STATE(flash_fsm, idle);
    }
}

// Handles SPI op and state machine
static void op_handler(int32_t status, uint32_t arg) {
    // Theoretically check errors
    next_state(NULL);
}

void flash_read(uint32_t addr, buffer_t dest, length_t words,
                callback_t on_complete) {}

void flash_write(uint32_t addr, buffer_t dest, length_t words,
                 callback_t on_complete) {}

void flash_erase(uint32_t addr, callback_t on_complete) {}

void flash_init() { FSM_SET_STATE(flash_fsm, reset); }

void flash_tx_complete_handler() {}

void flash_rx_complete_handler() {}
