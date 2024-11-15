#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32G030xx 

ifeq ($(DEBUG_PRINT), 1)
C_DEFS += -DDEBUG_PRINT
else
C_DEFS += -DRPC
endif

#######################################
# CFLAGS
#######################################
OPT = -Os

# cpu
CPU = -mcpu=cortex-m0plus

# fpu
# NONE for Cortex-M0/M0+/M3

# float-abi


# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

C_SOURCES += $(shell find board/$(BOARD)/src -name '*.c')
C_SOURCES +=  \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_adc.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_adc_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_adc.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rcc.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_dma.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_exti.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart_ex.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_crc.c \
board/$(BOARD)/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_crc_ex.c


# ASM sources
ASM_SOURCES +=  \
startup_stm32g030xx.s

C_INCLUDES += -Iboard/$(BOARD)/STM32G0xx_HAL_Driver/Inc \
-Iboard/$(BOARD)/STM32G0xx_HAL_Driver/Inc/Legacy \
-Iboard/$(BOARD)/CMSIS/Device/ST/STM32G0xx/Include \
-Iboard/$(BOARD)/CMSIS/Include


#######################################
# LDFLAGS
#######################################
# link script
LDFLAGS += -specs=nano.specs -specs=nosys.specs
LDSCRIPT = STM32G030C8Tx_FLASH.ld
LIBS += -lnosys
