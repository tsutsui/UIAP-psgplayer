all : flash

TARGET:=psg_play
ADDITIONAL_C_FILES+=ym2149_hw.c

TARGET_MCU?=CH32V003
include ../../ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean


