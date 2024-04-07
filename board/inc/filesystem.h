#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "defs.h"

#include <stddef.h>

void filesystem_init();
void file_open(const char* path);
size_t file_read(buffer_t dest, size_t size);
void file_seek(int seekv);
void file_close();

#endif // FILESYSTEM_H