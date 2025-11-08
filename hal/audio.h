#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // ========== Set audio ==========
    void audio_init();
    void audio_set_stream(buffer_t dma_buffer, length_t data_length,
                          callback_t on_complete);
    void audio_stream_start(void);
    void audio_stream_stop(void);
    bool audio_busy(void);
    void audio_shutdown(bool shutdown);

    void audio_stop(void);
    void audio_play(void);

#ifdef __cplusplus
}
#endif

#endif
