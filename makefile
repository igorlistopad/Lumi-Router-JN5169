# Makefile for the Lumi Router

# Application target name
TARGET = LumiRouter

# Target board: DGNWG05LM (Xiaomi) or ZHWG11LM (Aqara).
# Objects are shared in build/: run make clean before switching boards.
BOARD ?=
CFLAGS += -DBOARD_$(BOARD)

# Application version
VERSION_STRING ?= 26.10.0-beta2
CFLAGS         += -DVERSION_STRING=\"$(VERSION_STRING)\"

# Application build date
BUILD_DATE ?=
ifeq ($(strip $(BUILD_DATE)), )
	CFLAGS += -DBUILD_DATE_STRING=\"$(shell date -u +%Y%m%d)\"
else
	CFLAGS += -DBUILD_DATE_STRING=\"$(BUILD_DATE)\"
endif

# Network settings
# Channel (0 for default channels)
SINGLE_CHANNEL ?= 0
CFLAGS         += -DSINGLE_CHANNEL=$(SINGLE_CHANNEL)

# Target chip is the JN5169
JENNIC_CHIP        = JN5169
JENNIC_CHIP_FAMILY = JN516x

# Select the network stack (e.g. MAC, ZBPro, ZCL)
JENNIC_STACK = ZCL

# Default SDK is the IEEE802.15.4 SDK
JENNIC_SDK = JN-SW-4170

# Default MAC is the IEEE802.15.4 Mini MAC
JENNIC_MAC = MiniMacShim

# ZBPro Stack specific options
ZBPRO_DEVICE_TYPE = ZCR
PDM_BUILD_TYPE    =_EEPROM
STACK_SIZE        = 5000
MINIMUM_HEAP_SIZE = 2000
ZNCLKCMD = AppBuildZBPro.ld

# Green Power Proxy Basic sources
GP_SUPPORT = 1
APP_CLUSTERS_GREENPOWER_SRC = 1

# Debug options
DEBUG ?=
DEBUG_ENABLED := 0

ifeq ($(DEBUG), UART0)
	DEBUG_ENABLED := 1
	CFLAGS += -DDEBUG_UART_PORT=DBG_E_UART_0
else ifeq ($(DEBUG), UART1)
	DEBUG_ENABLED := 1
	CFLAGS += -DDEBUG_UART_PORT=DBG_E_UART_1
endif

ifeq ($(DEBUG_ENABLED), 1)
	TRACE   = 1
	CFLAGS += -DDEBUG_ENABLED
	CFLAGS += -DDEBUG_BDB
	CFLAGS += -DTRACE_APP=1
	CFLAGS += -DTRACE_GP=1
	CFLAGS += -DTRACE_REPORT=1
	CFLAGS += -DTRACE_ZCL=1
	CFLAGS += -DTRACE_UART=1
	CFLAGS += -DTRACE_SERIAL=1
	CFLAGS += -DTRACE_DEVICE_TEMPERATURE=1
endif

# BDB features – Enable as required
BDB_SUPPORT_NWK_STEERING ?= 1

# Generate build file name
ifneq ($(SINGLE_CHANNEL), 0)
	TARGET_FEATURES := $(TARGET_FEATURES)_CH$(SINGLE_CHANNEL)
endif
ifeq ($(DEBUG_ENABLED), 1)
	TARGET_FEATURES := $(TARGET_FEATURES)_DEBUG_$(DEBUG)
endif
ifneq ($(strip $(BUILD_DATE)), )
	TARGET_FEATURES := $(TARGET_FEATURES)_$(BUILD_DATE)
endif
GENERATED_FILE_NAME = $(TARGET)-$(BOARD)$(TARGET_FEATURES)

# Path definitions
APP_BASE = $(abspath .)
APP_BLD_DIR = $(APP_BASE)/build
APP_SRC_DIR = $(APP_BASE)/src
SDK_BASE_DIR ?= $(abspath ./sdk/$(JENNIC_SDK))
UTIL_SRC_DIR = $(COMPONENTS_BASE_DIR)/ZigbeeCommon/Source
HW_SRC_DIR = $(COMPONENTS_BASE_DIR)/HardwareAPI/Source
TOOL_COMMON_BASE_DIR ?= $(abspath ./tools)
TOOLCHAIN_PATH ?= ba2-toolchain
TOOLCHAIN_BASE_DIR = $(TOOL_COMMON_BASE_DIR)/$(TOOLCHAIN_PATH)

# Application Source files
# Note: Path to source file is found using vpath below, so only .c filename is required
APPSRC  = irq_JN516x.S
APPSRC += portasm_JN516x.S
APPSRC += port_JN516x.c
APPSRC += pdum_gen.c
APPSRC += pdum_apdu.S
APPSRC += zps_gen.c
APPSRC += app_start.c
APPSRC += app_main.c
APPSRC += app_router_node.c
APPSRC += app_zcl_task.c
APPSRC += app_green_power.c
APPSRC += app_reporting.c
APPSRC += app_serial_commands.c
APPSRC += app_device_temperature.c
APPSRC += app_uart.c
APP_ZPSCFG = app.zpscfg

# Standard Application header search paths
INCFLAGS += -I$(APP_SRC_DIR)
INCFLAGS += -I$(APP_SRC_DIR)/..

# Application specific include files
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/ZCL/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/Xcv/Include/
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/Recal/Include/

# Optional stack features enabled in app.zpscfg
OPTIONAL_STACK_FEATURES = $(shell $(ZPSCONFIG) -n $(TARGET) -f $(APP_SRC_DIR)/$(APP_ZPSCFG) -y)

# Configure for the selected chip or chip family
-include $(SDK_BASE_DIR)/Chip/Common/Build/config.mk
-include $(SDK_BASE_DIR)/Stack/Common/Build/config.mk
-include $(SDK_BASE_DIR)/Components/BDB/Build/config.mk

# LumiRouter does not use SDK Generic devices.
GENERIC_SRCS :=

TEMP = $(APPSRC:.c=.o)
APPOBJS_TMP = $(TEMP:.S=.o)
APPOBJS := $(addprefix $(APP_BLD_DIR)/,$(APPOBJS_TMP))

# Application dynamic dependencies
APPDEPS_TMP = $(APPOBJS_TMP:.o=.d)
APPDEPS := $(addprefix $(APP_BLD_DIR)/,$(APPDEPS_TMP))

# Linker
# Add application libraries before chip specific libraries to linker so
# symbols are resolved correctly (i.e. ordering is significant for GCC)
APPLDLIBS := $(foreach lib,$(APPLIBS),$(if $(wildcard $(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(addsuffix _$(JENNIC_CHIP).a,$(lib)))),$(addsuffix _$(JENNIC_CHIP),$(lib)),$(addsuffix _$(JENNIC_CHIP_FAMILY),$(lib))))
LDLIBS := $(APPLDLIBS) $(LDLIBS)

# Path to directories containing application source 
vpath % $(APP_SRC_DIR):$(ZCL_SRC_DIRS):$(BDB_SRC_DIR):$(UTIL_SRC_DIR):$(HW_SRC_DIR)

all: pre-build $(APP_BLD_DIR)/$(GENERATED_FILE_NAME).bin

$(APP_SRC_DIR)/pdum_gen.c $(APP_SRC_DIR)/pdum_gen.h $(APP_SRC_DIR)/pdum_apdu.S: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(PDUMCONFIG)
	$(info Configuring the PDUM ...)
	$(PDUMCONFIG) -z $(TARGET) -f $< -o $(APP_SRC_DIR)

$(APP_SRC_DIR)/zps_gen.c $(APP_SRC_DIR)/zps_gen.h: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(ZPSCONFIG)
	$(info Configuring the Zigbee Protocol Stack ...)
	$(ZPSCONFIG) -n $(TARGET) -t $(JENNIC_CHIP) -l $(ZPS_NWK_LIB) -a $(ZPS_APL_LIB) -c $(TOOLCHAIN_BASE_DIR) -f $< -o $(APP_SRC_DIR)

$(APP_BLD_DIR)/%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -o "$@" $(CFLAGS) $(INCFLAGS) "$<" -MD -MF "$(APP_BLD_DIR)/$*.d" -MP
	@echo

$(APP_BLD_DIR)/%.o: %.c 
	$(info Compiling $< ...)
	$(CC) -c -o "$@" $(CFLAGS) $(INCFLAGS) "$<" -MD -MF "$(APP_BLD_DIR)/$*.d" -MP
	@echo

$(APP_BLD_DIR)/$(GENERATED_FILE_NAME).elf: $(APPOBJS) $(addsuffix.a,$(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(APPLDLIBS))) 
	$(info Linking $@ ...)
	$(CC) -Wl,-u_AppColdStart -Wl,-u_AppWarmStart $(LDFLAGS) -T$(ZNCLKCMD) -o $@ -Wl,--start-group $(APPOBJS) $(addprefix -l,$(LDLIBS)) -lm -Wl,--end-group -Wl,-Map,$(APP_BLD_DIR)/$(GENERATED_FILE_NAME).map 
	$(SIZE) $@

$(APP_BLD_DIR)/$(GENERATED_FILE_NAME).bin: $(APP_BLD_DIR)/$(GENERATED_FILE_NAME).elf
	$(info Generating binary ...)
	$(OBJCOPY) -j .version -j .bir -j .flashheader -j .vsr_table -j .vsr_handlers -j .rodata -j .text -j .data -j .bss -j .heap -j .stack -S -O binary $< $@
	@echo

pre-build:
ifeq ($(BOARD), DGNWG05LM)
	$(info Target board: Xiaomi DGNWG05LM)
else ifeq ($(BOARD), ZHWG11LM)
	$(info Target board: Aqara ZHWG11LM)
else
	$(error Use make BOARD=DGNWG05LM or make BOARD=ZHWG11LM)
endif
ifeq ($(wildcard $(SDK_BASE_DIR)/Stack), )
	$(error Please check sdk directory)
endif
ifeq ($(wildcard $(TOOLCHAIN_BASE_DIR)/bin), )
	$(error BA2 toolchain is not installed. Please run: make install)
endif
	@mkdir -p $(APP_BLD_DIR)

clean:
	rm -f $(APPOBJS) $(APPDEPS) $(APP_BLD_DIR)/$(TARGET)*.map $(APP_BLD_DIR)/$(TARGET)*.elf $(APP_BLD_DIR)/$(TARGET)*.bin
	rm -f $(APP_SRC_DIR)/pdum_gen.* $(APP_SRC_DIR)/zps_gen.* $(APP_SRC_DIR)/pdum_apdu.S
	@echo

install: install-sdk install-toolchain
	$(info SDK and Toolchain installed.)
	@echo

install-sdk: $(SDK_BASE_DIR)/Stack

$(SDK_BASE_DIR)/Stack: $(SDK_BASE_DIR)
ifeq ($(wildcard $(SDK_BASE_DIR)/Stack), )
ifneq ($(shell git submodule status $(SDK_BASE_DIR) 2> /dev/null), )
	git submodule update --init
else
	curl -fL --retry 3 "https://github.com/igorlistopad/JN-SW-4170/archive/refs/heads/v1840.tar.gz" \
		-o "$(SDK_BASE_DIR)/../JN-SW-4170.tar.gz"
	tar -xzf $(SDK_BASE_DIR)/../JN-SW-4170.tar.gz --strip-components=1 -C $(SDK_BASE_DIR)
	rm $(SDK_BASE_DIR)/../JN-SW-4170.tar.gz
endif
endif

$(SDK_BASE_DIR):
	mkdir -p $(SDK_BASE_DIR)

install-toolchain: $(TOOLCHAIN_BASE_DIR)/bin

$(TOOLCHAIN_BASE_DIR)/bin: $(TOOLCHAIN_BASE_DIR)
ifeq ($(wildcard $(TOOLCHAIN_BASE_DIR)/bin), )
	@case "$$(uname -s)-$$(uname -m)" in \
		Linux-x86_64|Linux-amd64) platform=linux-amd64 ;; \
		Linux-aarch64|Linux-arm64) platform=linux-arm64 ;; \
		Darwin-x86_64) platform=macos-amd64 ;; \
		Darwin-arm64) platform=macos-arm64 ;; \
		MSYS*-x86_64|MINGW*-x86_64|CYGWIN*-x86_64) platform=windows-amd64 ;; \
		*) printf '%s\n' \
			"Unsupported toolchain platform" \
			"Try using a Linux Dev Container in Visual Studio Code" >&2; exit 1 ;; \
	esac; \
	curl -fL --retry 3 "https://github.com/igorlistopad/BA2-toolchain/releases/download/2026.9.12/ba2-toolchain-$$platform.tar.gz" \
		-o "$(TOOL_COMMON_BASE_DIR)/ba2-toolchain.tar.gz"
	tar -xzf "$(TOOL_COMMON_BASE_DIR)/ba2-toolchain.tar.gz" --strip-components=1 -C "$(TOOLCHAIN_BASE_DIR)"
	rm "$(TOOL_COMMON_BASE_DIR)/ba2-toolchain.tar.gz"
endif

$(TOOLCHAIN_BASE_DIR):
	mkdir -p $(TOOLCHAIN_BASE_DIR)

-include $(APPDEPS)

.NOTPARALLEL:
.PHONY: all clean install pre-build install-sdk install-toolchain
.DELETE_ON_ERROR:
