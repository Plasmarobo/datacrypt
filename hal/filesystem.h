#ifndef __FILESYSTEM_HAL_H_
#define __FILESYSTEM_HAL_H_

#include "defs.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define GAME_DB ("game.db")
#define WORDS_DB ("words.db")
#define AUDIO_DB ("audio.db")

    void filesystem_init();
    void file_open(const char *path);

    size_t file_read(buffer_t dest, size_t size);
    size_t file_write(buffer_t dest, size_t size);
    void file_rseek(int32_t seekv);
    void file_aseek(int32_t seekv);
    int32_t file_capacity();
    void file_close();
    void file_erase();

#ifdef __cplusplus
}
#endif

#endif // __FILESYSTEM_HAL_H_
