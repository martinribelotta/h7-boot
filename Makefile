TARGET = h7-boot
VERBOSE = n
DEBUG = 1
OPT = -Og
BUILD_DIR := build
LIBS := c m
LIBDIR := 
SPECS := nano nosys
C_DEFS := USE_FULL_LL_DRIVER USE_HAL_DRIVER STM32H750xx

# from https://stackoverflow.com/a/18258352
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

PREFIX := arm-none-eabi-
CC := $(PREFIX)gcc
AS := $(PREFIX)gcc -x assembler-with-cpp
CP := $(PREFIX)objcopy
SZ := $(PREFIX)size
HEX := $(CP) -O ihex
BIN := $(CP) -O binary -S
LST := $(PREFIX)objdump

MCU := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

C_INCLUDES := $(sort $(dir $(call rwildcard,.,*.h)))
C_SOURCES := $(filter-out %_template.c, $(call rwildcard,.,*.c))
ASM_SOURCES := $(call rwildcard,.,*.s)

COMMON_FLAGS := $(MCU) $(OPT) -Wall -fdata-sections -ffunction-sections

SPECS_FLAGS := $(addsuffix .specs,$(addprefix -specs=,$(SPECS)))

ASFLAGS := $(COMMON_FLAGS)

CFLAGS := $(COMMON_FLAGS)
CFLAGS += $(addprefix -D, $(C_DEFS))
CFLAGS += $(addprefix -I, $(C_INCLUDES))
CFLAGS += $(SPECS_FLAGS)

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information (not use :=, require delayed eval)
DEP_FLAGS = -MMD -MP -MF"$(@:%.o=%.d)"

LDSCRIPTS := STM32H750VBTx_FLASH.ld

LDFLAGS := $(MCU)
LDFLAGS += $(SPECS_FLAGS)
LDFLAGS += $(addprefix -T, $(LDSCRIPTS))
LDFLAGS += $(addprefix -L, $(LIBDIR))
LDFLAGS += $(addprefix -l, $(LIBS))
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage

SECTIONS := $(sort $(shell grep -oE '\.\S+\s*\:' $(LDSCRIPTS) | tr -d ':'))

LSTFLAGS := -xdS $(addprefix -j, $(SECTIONS))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).lst

OBJECTS = $(addprefix $(BUILD_DIR)/o/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/o/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))


ifeq ($(strip $(VERBOSE)),y)
Q=
else
Q=@
endif

# MKFILE:=Makefile
MKFILE:=

$(BUILD_DIR)/o/%.o: %.c $(MKFILE) | $(BUILD_DIR)
	@echo CC $<
	$(Q)$(CC) $(DEP_FLAGS) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/o/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/o/%.o: %.s $(MKFILE) | $(BUILD_DIR)
	@echo AS $<
	$(Q)$(AS) $(DEP_FLAGS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo LD $@
	$(Q)$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(Q)$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo HEX $@
	$(Q)$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo BIN $@
	$(Q)$(BIN) $< $@	

$(BUILD_DIR)/%.lst: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo LST $@
	$(Q)$(LST) $(LSTFLAGS) $< > $@

$(BUILD_DIR):
	$(Q)mkdir -p $@/o

clean:
	@echo CLEAN $(BUILD_DIR)
	$(Q)-rm -fR $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/o/*.d)

PHONY_TARGETS:=$(filter-out .%, $(shell grep -E '^.PHONY:' Makefile | cut -f 2 -d ':'))
-include vscode-integration.mk

.PHONY: all clean

.format:
	@echo CLANG FORMAT
	@echo clang-format $(wildcard Core/*/*.c) $(wildcard Core/*/*.h)
