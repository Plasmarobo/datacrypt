#include "defs.h"
#include "filesystem.h"

#include <sstream>

static FILE current_file;

void filesystem_init() {}

void file_open(const char *path)
{
    std::stringstream ss;
    ss << "./data" << path;
    current_file = fopen(ss.str().c_str(), "r+");
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
    fseek(current_file, seekv, SEEK_CUR);
}
void file_aseek(int32_t seekv)
{
    fseek(current_file, seekv, SEEK_SET);
}

int32_t file_capacity() { return -1; }

void file_close()
{
    fclose(current_file);
}
void file_erase()
{
    return 0;
}