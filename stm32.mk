# Common sources
C_SOURCES += $(shell find board -name '*.c')
C_SOURCES +=  \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_adc.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_adc_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_adc.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rcc.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_dma.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_exti.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart_ex.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_crc.c \
drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_crc_ex.c

# ASM sources
ASM_SOURCES =  \
startup_stm32g030xx.s

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-

# cpu
CPU = -mcpu=cortex-m0plus

# fpu
# NONE for Cortex-M0/M0+/M3

# float-abi
FLOAT_ABI= -mfloat-abi=soft

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT_ABI)

ASFLAGS = $(MCU)
CFLAGS = $(MCU) --specs=nano.specs

OPT = -Os

ifeq ($(DEBUG), 1)
CFLAGS += -g1 -gdwarf-2
endif

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32G030xx

C_INCLUDES += \
-Idrivers/STM32G0xx_HAL_Driver/Inc \
-Idrivers/STM32G0xx_HAL_Driver/Inc/Legacy \
-Idrivers/CMSIS/Device/ST/STM32G0xx/Include \
-Idrivers/CMSIS/Include

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32G030C8Tx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys
LDFLAGS = -static --specs=nano.specs $(MCU) -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
