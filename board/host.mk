#######################################
# binaries
#######################################
PREFIX =
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
C_DEFS =

#######################################
# CFLAGS
#######################################
OPT = -Og

# cpu
CPU =

# fpu
# NONE for Cortex-M0/M0+/M3
# float-abi

# mcu
MCU =

C_SOURCES += $(shell find board/$(BOARD)/src -name '*.c')
