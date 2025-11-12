
#######################################
# binaries
#######################################

QT_DIR := /usr/lib/qt6
QT_INCLUDE_DIR = /usr/include/x86_64-linux-gnu/qt6
QT_INCLUDE := -I$(QT_INCLUDE_DIR) -I$(QT_DIR)
PREFIX =

MOC := $(QT_DIR)/libexec/moc
RCC := $(QT_DIR)/libexec/rcc
UIC := $(QT_DIR)/libexec/uic

OBJ_DIR := $(BUILD_DIR)/objects
APP_DIR := $(BUILD_DIR)/apps
INC_DIR := $(BUILD_DIR)/include
MOC_DIR := $(BUILD_DIR)/moc

ASFLAGS =
CFLAGS = 
CXXFLAGS = $(QT_INCLUDE)

ifeq ($(DEBUG), 1)
CFLAGS += -g
endif

# C defines
C_DEFS =  \
-DSIMULATOR

C_INCLUDES += \
-Isimulator/inc

OPT = -Og

CXX_SOURCES = $(shell find simulator -name '*.cpp')
Q_SOURCES = $(shell find simulator -name '*.qml')
Q_SOURCES += $(shell find ui -name '*.qml')

#######################################
# LDFLAGS
#######################################
# link script

# libraries
LIBS = -lc -lm
LDFLAGS = $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref 
LDFLAGS += -L$(QT_DIR) -lQt6Gui -lQt6Core -lstdc++ -lm -lc -lgcc_s -lgcc

INCLUDE := -I$(QT_DIR)/qt6 -I$(QT_DIR)/qt6/QtQuick -I$(QT_DIR)/qt6/QtQuick.2 -I$(QT_DIR)/qt6/QtGui

#OBJECTS = $(addprefix $(OBJ_DIR)/,$(notdir $(Q_SOURCES:.qml=.qml.o)))
#vpath %.qml $(sort $(dir $(Q_SOURCES)))

# Generated files
UI_HEADER := $(INC_DIR)/ui_mainwindow.h
MOC_FILES := $(MOC_DIR)/moc_mainwindow.cpp
RESOURCE_FILES := $(BUILD_DIR)/qrc_resources.cpp

# Generate UI header from .ui file
#$(UI_HEADER): src/mainwindow.ui $(UIC)
#	$(UIC) $< -o $@

# Generate MOC file from header
#$(MOC_DIR)/moc_mainwindow.cpp: include/mainwindow.h $(MOC)
#	$(MOC) $(INCLUDE) $< -o $@

# Generate resource file from .qrc
$(RESOURCE_FILES): ui/SimulatorUI/SimulatorUI.qrc $(RCC) | $(BUILD_DIR)
	$(RCC) $< -o $@

# Compile QML files (if needed, typically handled by QtQuick)
$(BUILD_DIR)/%.qml.o: %.qml | $(BUILD_DIR)
	@mkdir -p $(@D)
	# QML files are typically compiled via QtQuick's runtime, not directly via g++
	# This is a placeholder; actual QML compilation is handled by QtQuick at runtime
	# For build-time processing, consider using QtQuickCompiler or similar tools