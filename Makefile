PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size
// edit the sdk path to where it is on your desktop
TARGET   := blink
BUILD    := build
LDSCRIPT := link.ld
SRCDIR   := src
SDK_PATH := /home/ian_m/mspm0-sdk

C_SRCS   := $(wildcard $(SRCDIR)/*.c)
SDK_SRCS := $(SDK_PATH)/source/ti/driverlib/dl_gpio.c \
            $(SDK_PATH)/source/ti/driverlib/dl_common.c

OBJS     := $(addprefix $(BUILD)/,$(notdir $(C_SRCS:.c=.o)))
OBJS     += $(addprefix $(BUILD)/,$(notdir $(SDK_SRCS:.c=.o)))

ELF      := $(BUILD)/$(TARGET).elf

CPU      := -mcpu=cortex-m0plus -mthumb
CFLAGS   := $(CPU) -Og -g3 -Wall -Wextra -ffunction-sections -fdata-sections -fno-common -ffreestanding
CFLAGS   += -I$(SDK_PATH)/source -I$(SDK_PATH)/source/third_party/CMSIS/Core/Include
CFLAGS   += -D__MSPM0L2228__

LDFLAGS  := $(CPU) -Og -g3 -T$(LDSCRIPT) -nostdlib -Wl,--gc-sections

OPENOCD      ?= openocd
OPENOCD_ARGS := -f interface/xds110.cfg -f target/ti/mspm0.cfg -c "adapter speed 2500"

all: $(ELF) size

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: $(SRCDIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/dl_%.o: $(SDK_PATH)/source/ti/driverlib/dl_%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJS) $(LDSCRIPT) | $(BUILD)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

size: $(ELF)
	@$(SIZE) $(ELF)

flash: $(ELF)
	$(OPENOCD) $(OPENOCD_ARGS) -c "program $(ELF) verify reset exit"

clean:
	rm -rf $(BUILD)
