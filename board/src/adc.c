#include "adc.h"

#include "bsp.h"
#include "scheduler.h"

#define ADC_INTERVAL_MS (100)

typedef enum {
    AS_IDLE = 0,
    AS_EBAT = 1,
    AS_RNG,
    AS_TEMP,
    AS_VBAT,
    AS_MAX
} adc_scan_t;

static callback_t adc_ready_cb = NULL;
static task_handle_t adc_task;
static adc_scan_t scan_state;
static struct {
    uint16_t ebat;
    uint16_t rng;
    uint16_t temp_raw;
    uint16_t vbat;
} adc_values;
static uint16_t* write_ptr;

void ADC_Select_BAT_READ() {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void ADC_Select_ANALOG_RNG() {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void ADC_Select_TEMP() {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_2;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void ADC_Select_VBAT() {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_VBAT;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_2;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

static void adc_scan(int32_t status) {
    HAL_ADC_Stop_IT(&hadc1);
    switch (scan_state) {
        case AS_IDLE:
            ADC_Select_BAT_READ();
            write_ptr = &adc_values.ebat;
            break;
        case AS_EBAT:
            ADC_Select_ANALOG_RNG();
            write_ptr = &adc_values.rng;
            break;
        case AS_RNG:
            ADC_Select_TEMP();
            write_ptr = &adc_values.temp_raw;
            break;
        case AS_TEMP:
            ADC_Select_VBAT();
            write_ptr = &adc_values.vbat;
            break;
        case AS_VBAT:  // intentional fallthrough
            write_ptr = NULL;
            if (NULL != adc_ready_cb) {
                adc_ready_cb(0);
            }
        default:
            scan_state = AS_IDLE;
            task_delayed_unique(adc_scan, MILLIS(ADC_INTERVAL_MS));
            return;
            break;
    }
    HAL_ADC_Start_IT(&hadc1);
}

void adc_init(callback_t on_ready) {
    adc_ready_cb = on_ready;
    scan_state = AS_IDLE;
    adc_task = task_delayed_unique(adc_scan, MILLIS(ADC_INTERVAL_MS));
}

// Reads external voltage divider
uint16_t bat_read() { return adc_values.ebat; }
// Floating analog pin
uint16_t rng_read() { return adc_values.rng; }
// Internal temperature sensor (converted)
uint16_t temp_read() { return 0; }
// Internal temperature sensor (raw)
uint16_t temp_read_raw() { return adc_values.temp_raw; }
// Internal vbat
uint16_t vbat_read() { return adc_values.vbat; }

void adc_handler(int32_t status) {
    if (write_ptr != NULL) {
        *write_ptr = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    scan_state += 1;
    task_immediate(adc_scan);
}
