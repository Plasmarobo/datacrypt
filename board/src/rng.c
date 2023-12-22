#include "bsp.h"

static uint8_t entropy;
static uint32_t seed;

void rng_init() {
    entropy = 0;
    seed = 0;
    HAL_ADC_Start_IT(&hadc1);
}

#define MAXRAN (0xFFFF)

uint16_t random() {
    const uint32_t a = 1103515245;
    const uint32_t c = 12345;
    const uint32_t modulus = 0x80000000;
    if (entropy > sizeof(seed)) {
        seed = (a * seed + c) % modulus;
        return seed & MAXRAN;
    }
    return 0;
}

void rng_update_handler(int32_t status) {
    seed = (seed << 1) | (HAL_ADC_GetValue(&hadc1) & 0x01);
    ++entropy;
    if (entropy > sizeof(seed)) {
        HAL_ADC_Stop(&hadc1);
    } else {
        HAL_ADC_Start_IT(&hadc1);
    }
}
