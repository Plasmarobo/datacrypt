#include "bsp.h"

// Audio amplifier
// AUDIO_SD and FAULT are tied together in hardware

void audio_shutdown(bool shutdown)
{
    // Drive audio_sd low
    gpio_set(AUDIO_SD, shutdown ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
