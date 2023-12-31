/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "bsp.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_spi2_rx;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart1;

gpio_t BAT_READ = {GPIO_PIN_0, GPIOA};
gpio_t LED_DATA = {GPIO_PIN_2, GPIOA};
gpio_t FLASH_DIN = {GPIO_PIN_3, GPIOA};
gpio_t FLASH_DOUT = {GPIO_PIN_4, GPIOA};
gpio_t LED_SCK = {GPIO_PIN_5, GPIOA};
gpio_t PWM_CH1 = {GPIO_PIN_6, GPIOA};
gpio_t PWM_CH2 = {GPIO_PIN_7, GPIOA};
gpio_t SUCCESS_SW = {GPIO_PIN_0, GPIOB};
gpio_t FAILURE_SW = {GPIO_PIN_1, GPIOB};
gpio_t INTERCEPT_SW = {GPIO_PIN_2, GPIOB};
gpio_t LOCK2_TGL = {GPIO_PIN_10, GPIOB};
gpio_t LOCK3_TGL = {GPIO_PIN_11, GPIOB};
gpio_t FLASH_CS = {GPIO_PIN_12, GPIOB};
gpio_t FLASH_SCK = {GPIO_PIN_13, GPIOB};
gpio_t WIN_SW = {GPIO_PIN_14, GPIOB};
gpio_t SHUFFLE_SW = {GPIO_PIN_15, GPIOB};
gpio_t TURN_SW = {GPIO_PIN_8, GPIOA};
gpio_t SOUND_TGL = {GPIO_PIN_6, GPIOC};
gpio_t MISC_SW = {GPIO_PIN_7, GPIOC};
gpio_t DISPLAY_SDA = {GPIO_PIN_10, GPIOA};
gpio_t HIDE_TGL = {GPIO_PIN_0, GPIOD};
gpio_t AUDIO_SD = {GPIO_PIN_2, GPIOD};
gpio_t TIMER_SW = {GPIO_PIN_3, GPIOB};
gpio_t MUXRST = {GPIO_PIN_5, GPIOB};
gpio_t DISPLAY_SCL = {GPIO_PIN_6, GPIOB};
gpio_t LOCK0_TGL = {GPIO_PIN_8, GPIOB};
gpio_t LOCK1_TGL = {GPIO_PIN_9, GPIOB};
gpio_t ANALOG_RNG = {GPIO_PIN_12, GPIOA};
/* USER CODE BEGIN Private defines */

/* USER CODE BEGIN PV */
static timespan_t us_accumulator;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI2_Init(void);
static void MX_ADC1_Init(void);
static void MX_CRC_Init(void);
static void MX_TIM14_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
static int primask_bit;

void enter_critical(void) {
    primask_bit = __get_PRIMASK(); /**< backup PRIMASK bit */
    __disable_irq(); /**< Disable all interrupts by setting PRIMASK bit on */
}
void exit_critical(void) {
    __set_PRIMASK(primask_bit); /**< Restore PRIMASK bit*/
}

/**
 * @brief  The application entry point.
 * @retval int
 */
void bsp_init(void) {
    /* USER CODE BEGIN 1 */
    us_accumulator = 0;
    /* USER CODE END 1 */

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_SPI1_Init();
    MX_TIM3_Init();
    MX_SPI2_Init();
    MX_ADC1_Init();
    MX_CRC_Init();
    MX_TIM14_Init();
    __HAL_FREEZE_TIM1_DBGMCU();
    __HAL_FREEZE_TIM3_DBGMCU();
    __HAL_FREEZE_TIM14_DBGMCU();
    /* Initialize interrupts */
    MX_NVIC_Init();

    rng_init();
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 9;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV3;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief NVIC Configuration.
 * @retval None
 */
static void MX_NVIC_Init(void) {
    /* FLASH_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(FLASH_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(FLASH_IRQn);
    /* EXTI0_1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
    /* EXTI2_3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
    /* EXTI4_15_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    /* DMA1_Channel1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    /* DMA1_Channel2_3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    /* DMA1_Ch4_5_DMAMUX1_OVR_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Ch4_5_DMAMUX1_OVR_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Ch4_5_DMAMUX1_OVR_IRQn);
    /* TIM3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    /* I2C1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(I2C1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_IRQn);
    /* SPI1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    /* SPI2_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);
    /* USART1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {
    /* USER CODE BEGIN ADC1_Init 0 */

    /* USER CODE END ADC1_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */

    /* USER CODE END ADC1_Init 1 */

    /** Configure the global features of the ADC (Clock, Resolution, Data
     * Alignment and number of conversion)
     */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_8B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.LowPowerAutoPowerOff = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_1CYCLE_5;
    hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
    hadc1.Init.OversamplingMode = DISABLE;
    hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN ADC1_Init 2 */

    /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void) {
    /* USER CODE BEGIN CRC_Init 0 */

    /* USER CODE END CRC_Init 0 */

    /* USER CODE BEGIN CRC_Init 1 */

    /* USER CODE END CRC_Init 1 */
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    if (HAL_CRC_Init(&hcrc) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN CRC_Init 2 */

    /* USER CODE END CRC_Init 2 */
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {
    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00602173;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }

    /** Configure Analogue filter
     */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) !=
        HAL_OK) {
        Error_Handler();
    }

    /** Configure Digital filter
     */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C1_Init 2 */

    /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {
    /* USER CODE BEGIN SPI1_Init 0 */

    /* USER CODE END SPI1_Init 0 */

    /* USER CODE BEGIN SPI1_Init 1 */

    /* USER CODE END SPI1_Init 1 */
    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void) {
    /* USER CODE BEGIN SPI2_Init 0 */

    /* USER CODE END SPI2_Init 0 */

    /* USER CODE BEGIN SPI2_Init 1 */

    /* USER CODE END SPI2_Init 1 */
    /* SPI2 parameter configuration*/
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_ENABLE;
    hspi2.Init.CRCPolynomial = 7;
    hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI2_Init 2 */

    /* USER CODE END SPI2_Init 2 */
}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void) {
    /* USER CODE BEGIN TIM3_Init 0 */

    /* USER CODE END TIM3_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* USER CODE BEGIN TIM3_Init 1 */

    /* USER CODE END TIM3_Init 1 */
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 255;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) !=
        HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) !=
        HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) !=
        HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM3_Init 2 */
    HAL_TIM_Base_Start_IT(&htim3);
    /* USER CODE END TIM3_Init 2 */
    HAL_TIM_MspPostInit(&htim3);
}

/**
 * @brief TIM14 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM14_Init(void) {
    /* USER CODE BEGIN TIM14_Init 0 */

    /* USER CODE END TIM14_Init 0 */

    /* USER CODE BEGIN TIM14_Init 1 */

    /* USER CODE END TIM14_Init 1 */
    htim14.Instance = TIM14;
    htim14.Init.Prescaler = 64;
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 65535;
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM14_Init 2 */
    HAL_TIM_Base_Start_IT(&htim14);
    /* USER CODE END TIM14_Init 2 */
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {
    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) !=
        HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) !=
        HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(AUDIO_SD.port, AUDIO_SD.pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(MUXRST.port, MUXRST.pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : PC13 PC14 PC15 */
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : PF0 PF1 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /*Configure GPIO pins : PA1 PA11 PA12 PA15 */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_11 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ANALOG_RNG.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ANALOG_RNG.port, &GPIO_InitStruct);

    /*Configure GPIO pins : SUCCESS_SW_Pin FAILURE_SW_Pin INTERCEPT_SW_Pin
       LOCK2_TGL_Pin TIMER_SW_Pin LOCK0_TGL_Pin LOCK1_TGL_Pin */
    GPIO_InitStruct.Pin = SUCCESS_SW.pin | FAILURE_SW.pin | INTERCEPT_SW.pin |
                          LOCK2_TGL.pin | TIMER_SW.pin | LOCK0_TGL.pin |
                          LOCK1_TGL.pin | LOCK3_TGL.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pins : LOCK3_TGL_Pin WIN_SW_Pin SHUFFLE_SW_Pin */
    GPIO_InitStruct.Pin = WIN_SW.pin | SHUFFLE_SW.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : TURN_SW_Pin */
    GPIO_InitStruct.Pin = TURN_SW.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(TURN_SW.port, &GPIO_InitStruct);

    /*Configure GPIO pins : SOUND_TGL_Pin MISC_SW_Pin */
    GPIO_InitStruct.Pin = SOUND_TGL.pin | MISC_SW.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pin : HIDE_TGL_Pin */
    GPIO_InitStruct.Pin = HIDE_TGL.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(HIDE_TGL.port, &GPIO_InitStruct);

    /*Configure GPIO pins : PD1 PD3 */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Configure GPIO pin : AUDIO_SD_Pin */
    GPIO_InitStruct.Pin = AUDIO_SD.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(AUDIO_SD.port, &GPIO_InitStruct);

    /*Configure GPIO pin : PB4 */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : MUXRST_Pin */
    GPIO_InitStruct.Pin = MUXRST.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MUXRST.port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM1) {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state
     */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
 * @brief This function handles TIM14 global interrupt.
 */
void TIM14_IRQHandler(void) {
    /* USER CODE BEGIN TIM14_IRQn 0 */

    /* USER CODE END TIM14_IRQn 0 */
    HAL_TIM_IRQHandler(&htim14);
    /* USER CODE BEGIN TIM14_IRQn 1 */
    // 0x10000 - 16 bit rollover
    us_accumulator += 0x10000;
    /* USER CODE END TIM14_IRQn 1 */
}

timespan_t microseconds() { return us_accumulator | TIM14->CNT; }

timespan_t milliseconds() { return microseconds() / 1000; }

// ========= Audio =============
void audio_set_stream(buffer_t dma_buffer, length_t data_length,
                      callback_t on_complete) {}

void audio_stream_start(void) {}

void audio_stream_stop(void) {}

bool audio_busy(void) {}

// ========== Read Switches ==========
static void gpio_event(gpio_t gpio) {
    bool current = gpio_get(gpio);
    if (gpio.value != current) {
        gpio.value = current;
        if (NULL != gpio.cb) {
            gpio.cb(gpio.value ? 1 : 0);
        }
    }
}

static callback_t serial_rxcomplete = NULL;
static callback_t serial_txcomplete = NULL;

void serial_read(buffer_t dest, length_t length, callback_t oncomplete) {
    serial_rxcomplete = oncomplete;
    HAL_UART_Receive_IT(&huart1, dest, length);
}

void serial_write(const buffer_t data, length_t length, callback_t oncomplete) {
    serial_txcomplete = oncomplete;
    HAL_UART_Transmit_IT(&huart1, data, length);
}

void serial_tx_complete_handler(int32_t status) {
    if (NULL != serial_txcomplete) {
        serial_txcomplete(status);
    }
}
void serial_rx_complete_handler(int32_t status) {
    if (NULL != serial_rxcomplete) {
        serial_rxcomplete(status);
    }
}

void gpio_update(int32_t status) {
    // reset IRQs
    uint8_t irq_flags = gpio_get_dirty();
    if ((0x01 << EXTI0_1_IRQn) & irq_flags) {
        gpio_event(SUCCESS_SW);
        gpio_event(FAILURE_SW);
        NVIC_EnableIRQ(EXTI0_1_IRQn);
        irq_flags &= ~EXTI0_1_IRQn;
    }
    if ((0x01 << EXTI2_3_IRQn) & irq_flags) {
        gpio_event(INTERCEPT_SW);
        gpio_event(TIMER_SW);
        NVIC_EnableIRQ(EXTI2_3_IRQn);
        irq_flags &= ~EXTI2_3_IRQn;
    }
    if ((0x01 << EXTI4_15_IRQn) & irq_flags) {
        gpio_event(LOCK0_TGL);
        gpio_event(LOCK1_TGL);
        gpio_event(LOCK2_TGL);
        gpio_event(LOCK3_TGL);
        NVIC_EnableIRQ(EXTI4_15_IRQn);
        irq_flags &= ~EXTI4_15_IRQn;
    }
    gpio_clear_dirty(irq_flags);
}

void gpio_set(gpio_t gpio, bool set) {
    HAL_GPIO_WritePin(gpio.port, gpio.pin, set ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool gpio_get(gpio_t gpio) {
    return HAL_GPIO_ReadPin(gpio.port, gpio.pin) == GPIO_PIN_SET;
}

void gpio_set_callback(gpio_t gpio, callback_t on_change) {
    gpio.cb = on_change;
}

static uint8_t dirty;

void gpio_mark_dirty(uint8_t fields) { dirty |= fields; }

void gpio_clear_dirty(uint8_t fields) { dirty &= ~(fields); }

uint8_t gpio_get_dirty() { return dirty; }

/* USER CODE END Private defines */
