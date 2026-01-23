#include "hal.h"
#include "defs.h"
#include "serial.h"
#include "print.h"
#include "simulator.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <optional>
#include <stdarg.h>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <sys/un.h>
#include <chrono>
#include <algorithm>

static std::atomic<bool> serial_running;
static std::queue<std::string> message_queue;
static std::mutex message_queue_mutex;
static std::mutex socket_mutex;
static std::optional<std::string> pending_message;
const char *socket_path = "/dev/Datacrypt";

static std::thread *sock_thread;
static std::thread *stdio_thread;
static std::thread *read_thread;
std::mutex read_mutex;
static callback_t read_callback = NULL;
static buffer_t read_buffer = NULL;
static length_t read_length = 0;

static std::optional<std::string> read_message()
{
    std::lock_guard<std::mutex> guard(message_queue_mutex);
    if (message_queue.empty())
    {
        return std::nullopt;
    }
    else
    {
        std::string message = message_queue.front();
        message_queue.pop();
        return message;
    }
}

static void write_message(std::string message)
{
    std::lock_guard<std::mutex> guard(message_queue_mutex);
    message_queue.push(message);
}

static void broadcast_message(std::string message)
{
    // For simulator, just print to stdout
    std::cout << message;
    SimulatorContext::getContext()->appendText(message.c_str(), message.length());
    // Overwrite pending message
    std::lock_guard<std::mutex> guard(socket_mutex);
    pending_message = message;
}

void serial_stdio_worker()
{
    std::string buffer;
    while (serial_running.load())
    {
        std::cin >> buffer;
        write_message(buffer.c_str());
        buffer.clear();
    }
}

void serial_sock_worker()
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/dev/datacrypt");
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 1);
    while (serial_running.load())
    {
        int connection_fd = accept(sock, NULL, NULL);
        while (connection_fd == -1)
        {
            char buffer[256];
            ssize_t bytes_read = read(connection_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0)
            {
                buffer[bytes_read] = '\0';
                write_message(std::string(buffer));
            }
            else if (bytes_read == 0)
            {
                // EOF
                break;
            }
            else if (errno != EAGAIN)
            {
                // Error
                std::cerr << "Error: " << strerror(errno) << std::endl;
                break;
            }
            std::lock_guard<std::mutex> guard(socket_mutex);
            if (pending_message.has_value())
            {
                size_t bytes_written = write(connection_fd, pending_message.value().c_str(), pending_message.value().length());
                if (bytes_written != pending_message.value().length())
                {
                    std::cerr << "Error: " << strerror(errno) << std::endl;
                    break;
                }
                pending_message = std::nullopt;
            }
        }
        close(connection_fd);
    }
    close(sock);
}

void serial_read_worker()
{
    std::string message;
    // Outer loop to read incoming data
    while (serial_running.load())
    {
        auto opt_message = read_message();
        if (opt_message.has_value())
        {
            message = opt_message.value();
            // Inner loop to copy complete messages
            while (message.length() > 0)
            {
                {
                    std::lock_guard<std::mutex> guard(read_mutex);
                    if (read_callback != NULL)
                    {
                        if (read_length > 0)
                        {
                            length_t to_copy = read_length > message.length() ? message.length() : read_length;
                            memcpy(read_buffer, message.c_str(), to_copy);
                            read_length -= to_copy;
                            read_buffer += to_copy;
                            if (read_length == 0)
                            {
                                read_callback(STATUS_OK);
                                read_callback = NULL;
                                read_buffer = NULL;
                                read_length = 0;
                            }
                            message = message.substr(to_copy);
                        }
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void serial_init()
{
    serial_running.store(true);
    sock_thread = new std::thread(&serial_sock_worker);
    stdio_thread = new std::thread(&serial_stdio_worker);
    read_thread = new std::thread(&serial_read_worker);
}

void serial_read(buffer_t dest, length_t length, callback_t oncomplete)
{
    std::lock_guard<std::mutex> guard(read_mutex);
    read_callback = oncomplete;
    read_buffer = dest;
    read_length = length;
}
void serial_write(const buffer_t data, length_t length, callback_t oncomplete)
{
    std::string message((const char *)data, length);
    broadcast_message(message);
    if (oncomplete)
    {
        oncomplete(length);
    }
}
void serial_print(const char *str)
{
    broadcast_message(std::string(str));
}

void serial_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    generic_vprintf(serial_write, fmt, args);
    va_end(args);
}

void vserial_printf(const char *fmt, va_list args)
{
    generic_vprintf(serial_write, fmt, args);
}
