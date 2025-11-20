
#######################################
# binaries
#######################################

QT_BIN_DIR := /usr/lib/qt6/libexec
QT_INCLUDE_DIR = /usr/include/x86_64-linux-gnu/qt6
QT_LIB_DIR = /usr/lib/x86_64-linux-gnu
PREFIX =

MOC := $(QT_BIN_DIR)/moc
RCC := $(QT_BIN_DIR)/rcc
UIC := $(QT_BIN_DIR)/uic

ASFLAGS =
CFLAGS = 
CXXFLAGS += -I$(QT_INCLUDE_DIR)

DEBUG = 1

ifeq ($(DEBUG), 1)
CFLAGS += -g
CXXFLAGS += -g
endif

# C defines
C_DEFS +=  \
-DSIMULATOR

QT_DEFS = \
-DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED

CXX_DEFS = \
$(QT_DEFS)

C_INCLUDES += \
-Isimulator/inc

OPT = -Og

CXX_SOURCES += $(shell find simulator -name '*.cpp')
Q_SOURCES = $(shell find simulator -name '*.qml')
Q_SOURCES += $(shell find ui -name '*.qml')

#######################################
# LDFLAGS
#######################################
# link script

# libraries
LIBS = -lc -lm
LDFLAGS = $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref 
LDFLAGS += -L$(QT_LIB_DIR) -lQt6Gui -lQt6Core -lQt6Quick -lQt6QuickWidgets -lQt6Qml -lQt6Widgets -lstdc++ -lm -lc -lgcc_s -lgcc

#RESOURCE_FILES := $(BUILD_DIR)/qrc_resources.cpp
RESOURCE_FILES = 
QT_HEADER_DIRS = simulator

# Collect header for MOC
HEADERS := $(shell find $(QT_HEADER_DIRS) -name *.h)
GENERATED_SOURCES += $(foreach hdr,$(HEADERS:.h=.moc.cpp),$(BUILD_DIR)/$(notdir $(hdr)))
GENERATED_OBJECTS := $(GENERATED_SOURCES:.cpp=.o)
RESOURCE_OBJECTS += $(RESOURCE_FILES:.cpp=.o)

$(info $(HEADERS))
$(info $(RESOURCE_FILES))
$(info $(GENERATED_SOURCES))
$(info $(GENERATED_OBJECTS))

OBJECTS += $(GENERATED_OBJECTS)
OBJECTS += $(RESOURCE_OBJECTS)

#######################################
# build the application
#######################################
vpath %.qml $(sort $(dir $(Q_SOURCES)))

$(GENERATED_SOURCES): $(HEADERS) $(MOC) | $(BUILD_DIR)
	$(MOC) $< $(DEFINES) $(INCLUDE) $ -o $@

$(RESOURCE_OBJECTS): $(RESOURCE_FILES) | $(BUILD_DIR)
	$(CXX) $< $(CXX_DEFINES) $(C_DEFINES) $(CXXFLAGS) $(COMMON_FLAGS) -c -o $@

$(GENERATED_OBJECTS): $(GENERATED_SOURCES) | $(BUILD_DIR)
	$(CXX) $< $(CXX_DEFINES) $(C_DEFINES) $(CXXFLAGS) $(COMMON_FLAGS) -c -o $@

# Generate resource file from .qrc
$(RESOURCE_FILES): ui/SimulatorUI/SimulatorUI.qrc $(RCC) | $(BUILD_DIR)
	$(RCC) -g cpp  $< -o $@

# Compile QML files (if needed, typically handled by QtQuick)
$(BUILD_DIR)/%.qml.o: %.qml | $(BUILD_DIR)
	@mkdir -p $(@D)
	# QML files are typically compiled via QtQuick's runtime, not directly via g++
	# This is a placeholder; actual QML compilation is handled by QtQuick at runtime
	# For build-time processing, consider using QtQuickCompiler or similar tools