
#ifndef __STM32_H_
#define __STM32_H_

#include "stm32g0xx_hal.h"

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
extern ADC_HandleTypeDef hadc1;

extern CRC_HandleTypeDef hcrc;

extern I2C_HandleTypeDef hi2c1;
extern DMA_HandleTypeDef hdma_i2c1_rx;
extern DMA_HandleTypeDef hdma_i2c1_tx;

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim14;

extern UART_HandleTypeDef huart1;

#endif
