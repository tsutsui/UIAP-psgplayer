#all : flash

TARGET:=psg_play
ADDITIONAL_C_FILES+=p6psg.c psg_driver.c tick2m.c ym2149_hw.c

TARGET_MCU?=CH32V003
UIAP_BOOTLOADER_VIDPID?=0x1209b803
FLASH_COMMAND?=$(MINICHLINK)/minichlink -c $(UIAP_BOOTLOADER_VIDPID) -w $< $(WRITE_SECTION) -b

CH32FUN_DIR?=./ch32fun/ch32fun
include $(CH32FUN_DIR)/ch32fun.mk

all: $(TARGET).bin
flash : cv_flash
clean : cv_clean
