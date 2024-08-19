#ifndef _EMULATOR_H_
#define _EMULATOR_H_

#include <stdint.h>

#include <optional>
#include <vector>

namespace Emulator {
// IPC API
// Serializable data structures representing hardware state

// GPIO
typedef enum {
    GPIO_PORT_A,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_F,
    GPIO_PORT_MAX,
} gpio_port_t;

// SPI/LED
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_info_t;

#define FLASH_MESSAGE_MAX 2048
// SPI/Flash
typedef struct {
    uint16_t page_address;
    uint16_t byte_address;
    uint16_t length;
    uint8_t packet[FLASH_MESSAGE_MAX];
} flash_info_t;

// Supports only 8 bit serial
class SerialChannel {
   private:
    uint32_t baud;
    std::vector<uint8_t> rx;
    std::vector<uint8_t> tx;
    std::optional<SerialChannel&> endpoint;

   public:
    SerialChannel();
    std::optional<std::vector<uint8_t>> receive();
    void send(std::slice<uint8_t>);
    void connect(SerialChannel& endpoint);
}

// I2C/Display

// Audio/PCM

// Filesystem

// ZMQ interface

class Emulator {
   private:
    uint32_t gpio_bitfield[GPIO_PORT_MAX];

}

// Emulator Engine
class Engine {
   private:
    std::atomic<bool> halt;
    std::thread worker;

   public:
    Emulator();
    ~Emulator();

    void start();
    void stop();
    void reset();
};
}  // namespace Emulator

#endif  // _EMULATOR_H_
