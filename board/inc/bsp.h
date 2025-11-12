/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "defs.h"
#include "hal.h"
#include "gpio.h"
#include "stm32g0xx.h"

    /* Private includes ----------------------------------------------------------*/
    /* USER CODE BEGIN Includes */

    /* USER CODE END Includes */

    /* Exported types ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */

    /* USER CODE END ET */

    /* Exported constants --------------------------------------------------------*/
    /* USER CODE BEGIN EC */

    /* USER CODE END EC */

    /* Exported macro ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

    /* Exported functions prototypes ---------------------------------------------*/
    void Error_Handler(void);

    /* USER CODE BEGIN EFP */

    /* USER CODE END EFP */

    /* Private defines -----------------------------------------------------------*/

    /* USER CODE BEGIN Private defines */
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

    // ========== Read Switches ==========

    // ========== Read External Flash ==========
    typedef uint16_t flash_page_address_t;

    void display_mux_enable();

    void flash_init(callback_t on_init);
    void flash_read(flash_page_address_t bp_addr, uint16_t byte_address_,
                    buffer_t dest, length_t size, callback_t on_complete);
    void flash_write(flash_page_address_t page, uint16_t byte_address_,
                     buffer_t data, length_t size, callback_t on_complete);
    void flash_commit(callback_t on_complete);
    void flash_update(flash_page_address_t page, uint16_t byte_address_,
                      buffer_t data, length_t size, callback_t on_complete);
    void flash_erase(uint32_t addr, callback_t on_complete);
    void flash_tx_complete_handler(int32_t status);
    void flash_rx_complete_handler(int32_t status);
    void serial_tx_complete_handler(int32_t status);
    void serial_rx_complete_handler(int32_t status);

    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
