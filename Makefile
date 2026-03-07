all : flash

TARGET:=psg_play
ADDITIONAL_C_FILES+=p6psg.c psg_driver.c tick2m.c ym2149_hw.c

TARGET_MCU?=CH32V003
include ../../ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean


