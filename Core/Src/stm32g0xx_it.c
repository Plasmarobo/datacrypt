/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32g0xx_it.c
 * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_it.h"

#include "bsp.h"
#include "i2c.h"
#include "scheduler.h"

#define GPIO_DEBOUNCE_US (25)
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_i2c1_rx;
extern DMA_HandleTypeDef hdma_i2c1_tx;
extern I2C_HandleTypeDef hi2c1;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim14;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
    while (1) {
    }
    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {
    /* USER CODE BEGIN HardFault_IRQn 0 */

    /* USER CODE END HardFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_HardFault_IRQn 0 */
        /* USER CODE END W1_HardFault_IRQn 0 */
    }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void) {
    /* USER CODE BEGIN SVC_IRQn 0 */

    /* USER CODE END SVC_IRQn 0 */
    /* USER CODE BEGIN SVC_IRQn 1 */

    /* USER CODE END SVC_IRQn 1 */
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void) {
    /* USER CODE BEGIN PendSV_IRQn 0 */

    /* USER CODE END PendSV_IRQn 0 */
    /* USER CODE BEGIN PendSV_IRQn 1 */

    /* USER CODE END PendSV_IRQn 1 */
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) {
    /* USER CODE BEGIN SysTick_IRQn 0 */

    /* USER CODE END SysTick_IRQn 0 */

    /* USER CODE BEGIN SysTick_IRQn 1 */

    /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32G0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g0xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles Flash global interrupt.
 */
void FLASH_IRQHandler(void) {
    /* USER CODE BEGIN FLASH_IRQn 0 */

    /* USER CODE END FLASH_IRQn 0 */
    HAL_FLASH_IRQHandler();
    /* USER CODE BEGIN FLASH_IRQn 1 */

    /* USER CODE END FLASH_IRQn 1 */
}

static void gpio_debounce(IRQn_Type IRQn) {
    // Avoid flooding IRQs
    NVIC_DisableIRQ(IRQn);
    gpio_mark_dirty(0x01 << IRQn);
    task_delayed_unique(&gpio_update, MICROS(GPIO_DEBOUNCE_US));
}

/**
 * @brief This function handles EXTI line 0 and line 1 interrupts.
 */
void EXTI0_1_IRQHandler(void) {
    /* USER CODE BEGIN EXTI0_1_IRQn 0 */

    /* USER CODE END EXTI0_1_IRQn 0 */
    HAL_GPIO_EXTI_IRQHandler(SUCCESS_SW.pin);
    HAL_GPIO_EXTI_IRQHandler(FAILURE_SW.pin);
    gpio_debounce(EXTI0_1_IRQn);
    /* USER CODE BEGIN EXTI0_1_IRQn 1 */

    /* USER CODE END EXTI0_1_IRQn 1 */
}

/**
 * @brief This function handles EXTI line 2 and line 3 interrupts.
 */
void EXTI2_3_IRQHandler(void) {
    /* USER CODE BEGIN EXTI2_3_IRQn 0 */

    /* USER CODE END EXTI2_3_IRQn 0 */
    HAL_GPIO_EXTI_IRQHandler(INTERCEPT_SW.pin);
    HAL_GPIO_EXTI_IRQHandler(TIMER_SW.pin);
    gpio_debounce(EXTI2_3_IRQn);
    /* USER CODE BEGIN EXTI2_3_IRQn 1 */

    /* USER CODE END EXTI2_3_IRQn 1 */
}

/**
 * @brief This function handles EXTI line 4 to 15 interrupts.
 */
void EXTI4_15_IRQHandler(void) {
    /* USER CODE BEGIN EXTI4_15_IRQn 0 */

    /* USER CODE END EXTI4_15_IRQn 0 */
    HAL_GPIO_EXTI_IRQHandler(LOCK0_TGL.pin);
    HAL_GPIO_EXTI_IRQHandler(LOCK1_TGL.pin);
    HAL_GPIO_EXTI_IRQHandler(LOCK2_TGL.pin);
    HAL_GPIO_EXTI_IRQHandler(LOCK3_TGL.pin);
    gpio_debounce(EXTI14_15_IRQHandler);
    /* USER CODE BEGIN EXTI4_15_IRQn 1 */

    /* USER CODE END EXTI4_15_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel 1 interrupt.
 */
void DMA1_Channel1_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

    /* USER CODE END DMA1_Channel1_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_i2c1_rx);
    /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

    /* USER CODE END DMA1_Channel1_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel 2 and channel 3 interrupts.
 */
void DMA1_Channel2_3_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Channel2_3_IRQn 0 */

    /* USER CODE END DMA1_Channel2_3_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_i2c1_tx);
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
    /* USER CODE BEGIN DMA1_Channel2_3_IRQn 1 */

    /* USER CODE END DMA1_Channel2_3_IRQn 1 */
}

/**
 * @brief This function handles DMA1 channel 4, channel 5 and DMAMUX1
 * interrupts.
 */
void DMA1_Ch4_5_DMAMUX1_OVR_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Ch4_5_DMAMUX1_OVR_IRQn 0 */

    /* USER CODE END DMA1_Ch4_5_DMAMUX1_OVR_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_spi1_tx);
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
    /* USER CODE BEGIN DMA1_Ch4_5_DMAMUX1_OVR_IRQn 1 */

    /* USER CODE END DMA1_Ch4_5_DMAMUX1_OVR_IRQn 1 */
}

/**
 * @brief This function handles ADC1 interrupt.
 */
void ADC1_IRQHandler(void) {
    /* USER CODE BEGIN ADC1_IRQn 0 */

    /* USER CODE END ADC1_IRQn 0 */
    HAL_ADC_IRQHandler(&hadc1);
    /* USER CODE BEGIN ADC1_IRQn 1 */

    /* USER CODE END ADC1_IRQn 1 */
}

/**
 * @brief This function handles TIM1 break, update, trigger and commutation
 * interrupts.
 */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) {
    /* USER CODE BEGIN TIM1_BRK_UP_TRG_COM_IRQn 0 */

    /* USER CODE END TIM1_BRK_UP_TRG_COM_IRQn 0 */
    HAL_TIM_IRQHandler(&htim1);
    /* USER CODE BEGIN TIM1_BRK_UP_TRG_COM_IRQn 1 */

    /* USER CODE END TIM1_BRK_UP_TRG_COM_IRQn 1 */
}

/**
 * @brief This function handles TIM3 global interrupt.
 */
void TIM3_IRQHandler(void) {
    /* USER CODE BEGIN TIM3_IRQn 0 */

    /* USER CODE END TIM3_IRQn 0 */
    HAL_TIM_IRQHandler(&htim3);
    /* USER CODE BEGIN TIM3_IRQn 1 */

    /* USER CODE END TIM3_IRQn 1 */
}

// TIM14 defined in bsp.c

/**
 * @brief This function handles I2C1 event global interrupt / I2C1 wake-up
 * interrupt through EXTI line 23.
 */
void I2C1_IRQHandler(void) {
    /* USER CODE BEGIN I2C1_IRQn 0 */

    /* USER CODE END I2C1_IRQn 0 */
    if (hi2c1.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR)) {
        HAL_I2C_ER_IRQHandler(&hi2c1);
    } else {
        HAL_I2C_EV_IRQHandler(&hi2c1);
    }
    /* USER CODE BEGIN I2C1_IRQn 1 */

    /* USER CODE END I2C1_IRQn 1 */
}

/**
 * @brief This function handles SPI1 global interrupt.
 */
void SPI1_IRQHandler(void) {
    /* USER CODE BEGIN SPI1_IRQn 0 */

    /* USER CODE END SPI1_IRQn 0 */
    HAL_SPI_IRQHandler(&hspi1);
    /* USER CODE BEGIN SPI1_IRQn 1 */

    /* USER CODE END SPI1_IRQn 1 */
}

/**
 * @brief This function handles SPI2 global interrupt.
 */
void SPI2_IRQHandler(void) {
    /* USER CODE BEGIN SPI2_IRQn 0 */

    /* USER CODE END SPI2_IRQn 0 */
    HAL_SPI_IRQHandler(&hspi2);
    /* USER CODE BEGIN SPI2_IRQn 1 */

    /* USER CODE END SPI2_IRQn 1 */
}

/**
 * @brief This function handles USART1 global interrupt / USART1 wake-up
 * interrupt through EXTI line 25.
 */
void USART1_IRQHandler(void) {
    /* USER CODE BEGIN USART1_IRQn 0 */

    /* USER CODE END USART1_IRQn 0 */
    HAL_UART_IRQHandler(&huart1);
    /* USER CODE BEGIN USART1_IRQn 1 */

    /* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* spi) {
    if (SPI1 == spi) {
        // Unlock LEDS
        leds_tx_complete_handler();
    }

    if (SPI2 == spi) {
        // Unlock flash chip perihperal, pump statemachine
        flash_tx_complete_handler();
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* spi) {
    if (SPI1 == spi) {
        // We shouldn't get a reply from the leds...
        leds_error_handler();
    }

    if (SPI2 == spi) {
        // Flash chip has sent us data
        flash_rx_complete_handler();
    }
}

void HAL_I2C_TxCpltCallback(I2C_HandleTypeDef* i2c) { i2c_complete_handler(); }

void HAL_I2C_RxCpltCallback(I2C_HandleTypeDef* i2c) { i2c_complete_handler(); }

/* USER CODE END 1 */
