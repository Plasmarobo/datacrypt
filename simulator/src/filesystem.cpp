#include "defs.h"
#include "filesystem.h"

#include <unistd.h>
#include <fcntl.h>
#include <string>

static int current_file;

void filesystem_init() {}

void file_open(const char *path)
{
    std::string sim_path = std::string("./data") + std::string(path);
    current_file = open(sim_path.c_str(), O_RDWR | O_CREAT);
}

size_t file_read(buffer_t dest, size_t size)
{
    return read(current_file, dest, size);
}
size_t file_write(buffer_t dest, size_t size)
{
    return write(current_file, dest, size);
}

void file_rseek(int32_t seekv)
{
    lseek(current_file, seekv, SEEK_CUR);
}

void file_aseek(int32_t seekv)
{
    lseek(current_file, seekv, SEEK_SET);
}

int32_t file_capacity() { return -1; }

void file_close()
{
    close(current_file);
}
void file_erase()
{
    return;
}