# STM32F429I-DISC1 bare-metal trace workload (GCC, no HAL).
#   make            build build/f429_trace.elf (DEBUG=1, OPT=-O0)
#   make flash      program with OpenOCD (ST-LINK on the board)
#   make clean
# CMSIS comes from Drivers/ (git clone --depth 1 of STMicroelectronics/cmsis_device_f4
# and STMicroelectronics/cmsis_core; the directory is git-ignored).
TARGET  = f429_trace
BUILD   = build
OPT    ?= -O0
GCC_PATH ?= /usr/bin
PREFIX  = $(GCC_PATH)/arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size
DEV     = Drivers/cmsis_device_f4
CORE    = Drivers/cmsis_core/CMSIS/Core
CPU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
DEFS    = -DSTM32F429xx -DHSE_VALUE=8000000U
INCS    = -I$(DEV)/Include -I$(CORE)/Include
CFLAGS  = $(CPU) $(DEFS) $(INCS) $(OPT) -g3 -Wall -fdata-sections -ffunction-sections
ASFLAGS = $(CPU) -x assembler-with-cpp -g3
LDFLAGS = $(CPU) -specs=nano.specs -specs=nosys.specs -TSTM32F429ZITx_FLASH.ld -Wl,-Map=$(BUILD)/$(TARGET).map,--cref -Wl,--gc-sections
SRCS    = src/main.c $(DEV)/Source/Templates/system_stm32f4xx.c
ASMS    = $(DEV)/Source/Templates/gcc/startup_stm32f429xx.s
OBJS    = $(addprefix $(BUILD)/,$(notdir $(SRCS:.c=.o) $(ASMS:.s=.o)))
vpath %.c $(sort $(dir $(SRCS)))
vpath %.s $(sort $(dir $(ASMS)))
all: $(BUILD)/$(TARGET).elf
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
$(BUILD)/%.o: %.s | $(BUILD)
	$(CC) -c $(ASFLAGS) $< -o $@
$(BUILD)/$(TARGET).elf: $(OBJS) STM32F429ZITx_FLASH.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@
$(BUILD):
	mkdir -p $@
flash: $(BUILD)/$(TARGET).elf
	openocd -f board/stm32f429disc1.cfg -c "program $< verify reset exit"
clean:
	rm -rf $(BUILD)
-include $(wildcard $(BUILD)/*.d)
.PHONY: all flash clean
