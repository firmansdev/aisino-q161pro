################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Card.c \
../src/EmvCommon.c \
../src/Nobu.c \
../src/display.c \
../src/file.c \
../src/func.c \
../src/httpDownload.c \
../src/main.c \
../src/monitor.c \
../src/mqtt.c \
../src/network.c \
../src/nobu_nfc.c \
../src/number_helper.c \
../src/param.c \
../src/sound.c \
../src/tms.c \
../src/tms_base64.c \
../src/tms_md5.c \
../src/tms_tms.c \
../src/utils.c 

OBJS += \
./src/Card.o \
./src/EmvCommon.o \
./src/Nobu.o \
./src/display.o \
./src/file.o \
./src/func.o \
./src/httpDownload.o \
./src/main.o \
./src/monitor.o \
./src/mqtt.o \
./src/network.o \
./src/nobu_nfc.o \
./src/number_helper.o \
./src/param.o \
./src/sound.o \
./src/tms.o \
./src/tms_base64.o \
./src/tms_md5.o \
./src/tms_tms.o \
./src/utils.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Q161Pro Compiler'
	c:\gcc-arm-none-eabi\bin\arm-none-eabi-gcc -std=gnu11 -mcpu=cortex-a5 -mtune=generic-armv7-a -mthumb -mfpu=neon-vfpv4 -mfloat-abi=hard -mno-unaligned-access -g -Os -Wall -fno-strict-aliasing -ffunction-sections -fdata-sections -I"c:\tools\NOV30\include" -I"c:\gcc-arm-none-eabi\libc\include" -I"c:\gcc-arm-none-eabi\lib\gcc\arm-none-eabi\7.2.1\include" -I"c:\tools\NOV30\vpostype\Q161Pro\include" -IC:\Users\Boy\eclipse-workspace\Q161Pro-Project\Q161ProDemo\Q161ProV1\inc -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/Card.o ./src/EmvCommon.o ./src/Nobu.o ./src/display.o ./src/file.o ./src/func.o ./src/httpDownload.o ./src/main.o ./src/monitor.o ./src/mqtt.o ./src/network.o ./src/nobu_nfc.o ./src/number_helper.o ./src/param.o ./src/sound.o ./src/tms.o ./src/tms_base64.o ./src/tms_md5.o ./src/tms_tms.o ./src/utils.o

.PHONY: clean-src

