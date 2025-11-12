#include "hal.h"
#include "audio.h"

void audio_init() {}
void audio_set_stream(buffer_t dma_buffer, length_t data_length,
                      callback_t on_complete)
{
    UNUSED(dma_buffer);
    UNUSED(data_length);

    on_complete(0);
}

void audio_stream_start(void)
{
}

void audio_stream_stop(void) {}

bool audio_busy(void)
{
    return true;
}

void audio_shutdown(bool shutdown)
{
    UNUSED(shutdown);
}

void audio_stop(void)
{
}

void audio_play(void)
{
}
