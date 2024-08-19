#include "audio.h"

#include "bsp.h"
#include "stm32.h"

void audio_init(void) {}

void audio_stop(void) {
    HAL_TIM_OC_Stop_IT(&htim3, 1);
    HAL_TIM_OC_Stop_IT(&htim3, 2);
}

void audio_play(void) {
    HAL_TIM_OC_Start_IT(&htim3, 1);
    HAL_TIM_OC_Start_IT(&htim3, 2);
}
