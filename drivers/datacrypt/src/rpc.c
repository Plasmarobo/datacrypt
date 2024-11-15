#include "rpc.h"

#include "bsp.h"
#include "scheduler.h"
#include "flash.h"

#ifdef RPC

#define RPC_TIMEOUT_MS (5000)

static rpc_state_t rpc_state;
static rpc_t rpc_buffer;
static uint8_t data_buffer[RPC_MAX_LENGTH];
static task_handle_t rpc_timeout_task;

static void rpc_exec(int32_t status);

// RPC is call response
// If the RPC receives any data, it will respond with a status packet (rpc_t)
// that contains the data To communicate with the RPC, you must send an rpc_t
// structure (6 bytes) The RPC subsystem will respond, reporting it's status If
// the status is okay and the command allows streaming data Bytes may then be
// streamed up to the length in the command or RPC_MAX_LENGTH, whichever is
// smaller The RPC subsystem will then send a status confirmation packet

static void rpc_send_status(int32_t status) {
    uint8_t rpc_status = (uint8_t)(status & 0xFF);
    serial_write(&rpc_status, sizeof(uint8_t), NULL);
}

static void rpc_finish(int32_t status) {
    rpc_send_status(status);
    if (rpc_timeout_task != NULL) {
        task_abort(rpc_timeout_task);
        rpc_timeout_task = NULL;
    }
    serial_flush_rx();
    rpc_state = RPC_READ_COMMAND;
}

// Will cascade into rpc finish
static void rpc_timeout_handler(int32_t status) {
    serial_flush_rx();
    rpc_state = RPC_TIMEOUT;
    rpc_timeout_task = NULL;
}

static void rpc_start_timeout(int32_t status) {
    rpc_timeout_task =
        task_delayed_unique(rpc_timeout_handler, MILLIS(RPC_TIMEOUT_MS));
}

static void rpc_continue(int32_t status) {
    rpc_send_status(status);
    if (rpc_timeout_task == NULL) {
        rpc_start_timeout(0);
    }
}

static int32_t rpc_do_command() {
    rpc_state = RPC_EXEC;
    future_t future;
    int32_t status;
    switch (rpc_buffer.code) {
        case RPC_READ_FLASH:
            WITH_FUTURE(flash_read(rpc_buffer.address, data_buffer,
                                   rpc_buffer.length, future),
                        MILLIS(RPC_TIMEOUT_MS));
            if (status != STATUS_SUCCESS) {
                rpc_finish(status);
            } else {
                rpc_send_status(RPC_STATUS_OK);
                serial_write(data_buffer, rpc_buffer.length, rpc_finish);
            }
            break;
        case RPC_WRITE_FLASH:
            flash_update(rpc_buffer.address,
                         data_buffer, rpc_buffer.length, rpc_finish);
            break;
        case RPC_COMMIT_FLASH:
            flash_commit(rpc_finish);
            break;
        case RPC_ERASE_FLASH:
            flash_erase(rpc_buffer.address, rpc_finish);
            break;
        case RPC_ECHO:
            display_clear();
            display_set_text(8, 8, data_buffer, rpc_buffer.length);
            display_show((uint8_t)rpc_buffer.address, rpc_finish);
            break;
        case RPC_INFO:
            display_clear();
            display_set_text(8, 8, VERSION_STRING, strlen(VERSION_STRING));
            display_show((uint8_t)rpc_buffer.address, rpc_finish);
            break;
        default:
            rpc_finish(RPC_STATUS_ERR_ARG);
            break;
    }
    return RPC_STATUS_OK;
}

static void rpc_exec(int32_t status) {
    serial_read((buffer_t)&rpc_buffer, sizeof(rpc_t), rpc_exec);
    if (status == RPC_STATUS_OK) {
        switch (rpc_state) {
            case RPC_READ_COMMAND:
                if (rpc_buffer.length <= RPC_MAX_LENGTH) {
                    // Start read of data
                    if (rpc_buffer.code == 'w' || rpc_buffer.code == 'p') {
                        rpc_state = RPC_READ_DATA;
                        serial_read(data_buffer, rpc_buffer.length, rpc_exec);
                        rpc_send_status(RPC_STATUS_OK);
                        rpc_start_timeout(0);
                    } else {
                        rpc_do_command();
                    }
                } else {
                    serial_flush_rx();
                    rpc_finish(RPC_STATUS_ERR_ARG);
                }
                break;
            case RPC_READ_DATA:
                rpc_do_command();
                break;
            case RPC_EXEC:
                rpc_continue(RPC_STATUS_BUSY);
                break;
            case RPC_TIMEOUT:
                rpc_finish(RPC_STATUS_ERR_TIMEOUT);
                break;
            default:
                rpc_finish(RPC_STATUS_ERR_ARG);
                break;
        }
    } else if (status == RPC_STATUS_BUSY) {
        rpc_finish(RPC_STATUS_BUSY);
    } else {
        rpc_finish(RPC_STATUS_ERR_EXEC);
    }
}
#endif

void rpc_init(void) {
#ifdef RPC
    rpc_state = RPC_READ_COMMAND;
    serial_read((buffer_t)&rpc_buffer, sizeof(rpc_t), rpc_exec);
#endif
}
