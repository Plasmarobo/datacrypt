#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#include "defs.h"

void adc_handler(int32_t status);
void adc_init(callback_t on_ready);

void ADC_Select_BAT_READ();
void ADC_Select_ANALOG_RNG();
void ADC_Select_TEMP();
void ADC_Select_VBAT();
#endif  // ADC_H
