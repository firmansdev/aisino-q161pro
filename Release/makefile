################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := c:\tools\build\make381\rm -f

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Q161Pro
BUILD_ARTIFACT_EXTENSION := img
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
RAW_BINARY += \
Q161Pro.elf \


# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: Q161Pro.img

# Tool invocations
Q161Pro.img: $(RAW_BINARY) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: Q161Pro Binary Packer'
	c:\tools\NOV30\vpostype\Q161Pro\dtools  mkappimg $(RAW_BINARY) "Q161Pro.img"
	@echo 'Finished building target: $@'
	@echo ' '

Q161Pro.elf: $(OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Q161Pro Linker'
	c:\gcc-arm-none-eabi\bin\arm-none-eabi-ld -o "Q161Pro.elf" $(OBJS) "C:\tools\NOV30\vpostype\Q161Pro\inits_L610_CN_16000.1100.00.86.69.03\core_stub.o" "C:\tools\NOV30\vpostype\Q161Pro\lib\libQ161Pro_L610_ApiCore_20250410.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\Q161Pro_master_L610_LIB_Hw0.1_Hw1.1_V1.02.34_Release_20250527_ff67b49d.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_FT.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_NFT.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\libQ161Pro_L610_QREncode.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\libQ161Pro_L610_ImageCore.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\libQ161Pro_L610_VSTCrypto.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\libQ161Pro_L610_MQTTClient.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\libQ161Pro_VTMS.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_paypass.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_paywave.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_amex.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_dpas.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_emv.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_jcb.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_pure.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_rupay.a" "C:\tools\NOV30\vpostype\Q161Pro\lib\lib_commonUniversal.a" "c:\gcc-arm-none-eabi\libc\libc.a" "c:\gcc-arm-none-eabi\libc\libm.a" "c:\gcc-arm-none-eabi\lib\gcc\arm-none-eabi\7.2.1\thumb\v7-ar\fpv3\hard\libgcc.a" -Map Q161Pro.map -nostdlib --gc-sections -T c:\tools\NOV30\vpostype\Q161Pro\inits_L610_CN_16000.1100.00.86.69.03\app_flashimg.ld
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Q161Pro.elf Q161Pro.img
	-@echo ' '

post-build:
	-c:\tools\NOV30\vpostype\Q161Pro\pack Q161Pro c:\tools & c:\tools/NOV30/vpostype/Q161Pro/VanstoneSign Q161Pro.img
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
