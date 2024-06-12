#ifndef RPC_H
#define RPC_H

#include <stdint.h>

#define RPC_MAX_LENGTH (32)

// Remote proc interface
// READ FLASH: rf u32address u32size -> data
#define RPC_READ_FLASH ('r')
// WRITE FLASH: wf u32address u32size data... -> status
#define RPC_WRITE_FLASH ('w')
#define RPC_COMMIT_FLASH ('c')
// ERASE FLASH: ef u32address u32size (rounded to blocks of 64 * 2kB) -> status
#define RPC_ERASE_FLASH ('e')
// print u8size ascii_data...
#define RPC_ECHO ('p')
// dump software info
#define RPC_INFO ('i')

#define RPC_STATUS_OK (0)
#define RPC_STATUS_BUSY (1)
#define RPC_STATUS_ERR_EXEC (2)
#define RPC_STATUS_ERR_ARG (3)

typedef enum {
    RPC_READ_COMMAND,
    RPC_READ_DATA,
    RPC_EXEC,
} rpc_state_t;

typedef struct __attribute__((packed)) {
    char code;
    uint32_t address;
    uint8_t length;
} rpc_t;

void rpc_init(void);

#endif
