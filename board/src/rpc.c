#include "rpc.h"

#include "bsp.h"

static rpc_state_t rpc_state;
static rpc_t rpc_buffer;
static uint8_t data_buffer[RPC_MAX_LENGTH];

static void rpc_exec(int32_t status);

static void rpc_finish(int32_t status) {
    serial_printf("RPC: %d\n", status);
    rpc_state = RPC_READ_COMMAND;
    serial_read((buffer_t)&rpc_buffer, sizeof(rpc_t), rpc_exec);
}

static int32_t rpc_do_command() {
    rpc_state = RPC_EXEC;
    switch (rpc_buffer.code) {
        case 'r':
            flash_read(rpc_buffer.address >> 16, rpc_buffer.address & 0xFFFF,
                       data_buffer, rpc_buffer.length, rpc_finish);
            break;
        case 'w':
            /*flash_update(rpc.address >> 16, rpc.address & 0xFFFF, data_buffer,
                         rpc_buffer.length, rpc_finish);*/
            rpc_finish(RPC_STATUS_ERR_EXEC);
            break;
        case 'c':
            flash_commit(rpc_finish);
            break;
        case 'e':
            /*flash_erase(rpc.address, rpc_finish);*/
            rpc_finish(RPC_STATUS_ERR_EXEC);
            break;
        case 'p':
            display_clear();
            draw_text(8, 8, data_buffer, rpc_buffer.length);
            display_show((uint8_t)rpc_buffer.address, rpc_finish);
            break;
        default:
            rpc_finish(RPC_STATUS_ERR_ARG);
            break;
    }
    return STATUS_OK;
}

static void rpc_exec(int32_t status) {
    switch (rpc_state) {
        case RPC_READ_COMMAND:
            if (rpc_buffer.length < RPC_MAX_LENGTH) {
                // Start read of data
                if (rpc_buffer.code == 'w' || rpc_buffer.code == 'p') {
                    status = RPC_STATUS_OK;
                    rpc_state = RPC_READ_DATA;
                    serial_read(data_buffer, rpc_buffer.length, rpc_exec);
                } else {
                    status = rpc_do_command();
                }
            } else {
                rpc_finish(RPC_STATUS_ERR_ARG);
            }
            break;
        case RPC_READ_DATA:
            status = rpc_do_command();
            break;
        case RPC_EXEC:
            status = RPC_STATUS_BUSY;
            serial_printf("RPC: %d\n", status);
            break;
        default:
            rpc_finish(RPC_STATUS_ERR_ARG);
            break;
    }
}

void rpc_init(void) {
    rpc_state = RPC_READ_COMMAND;
    serial_print("RPC Ready\n");
    serial_read((buffer_t)&rpc_buffer, sizeof(rpc_t), rpc_exec);
}
