################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/TI/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./src/HDC1080.obj" \
"./src/OPT3001.obj" \
"./src/app_utils.obj" \
"./src/device_status.obj" \
"./src/diskio.obj" \
"./src/ff.obj" \
"./src/gpio_if.obj" \
"./src/gps_if.obj" \
"./src/hooks.obj" \
"./src/i2c_if.obj" \
"./src/internet_if.obj" \
"./src/main.obj" \
"./src/network_common.obj" \
"./src/pinmux.obj" \
"./src/pms_if.obj" \
"./src/simplelink_if.obj" \
"./src/smartconfig.obj" \
"./src/startup_ccs.obj" \
"./src/timer_if.obj" \
"./src/uart_if.obj" \
"../cc3200v1p32.cmd" \
$(GEN_CMDS__FLAG) \
-llibc.a \
-lwebclient.a \
-ljson.a \
-ldriverlib.a \
-lota.a \
-lflc.a \
-lsimplelink.a \
-lfree_rtos.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include src/subdir_vars.mk
-include subdir_rules.mk
-include src/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
airu-firmware.out \

EXE_OUTPUTS__QUOTED += \
"airu-firmware.out" \

BIN_OUTPUTS += \
airu-firmware.bin \

BIN_OUTPUTS__QUOTED += \
"airu-firmware.bin" \


# All Target
all: airu-firmware.out

# Tool invocations
airu-firmware.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Linker'
	"C:/TI/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me -O4 --opt_for_speed=0 --define=ccs --define=__SL__ --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi -z -m"airu-firmware.map" --heap_size=0x5000 --stack_size=0x2000 -i"C:/TI/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/lib" -i"C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/ota/ccs/release" -i"C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/flc/ccs/release" -i"C:/TI/CC3200SDK_1.3.0/cc3200-sdk/netapps/http/client/ccs/HTTPClientMinLib/" -i"C:/TI/CC3200SDK_1.3.0/cc3200-sdk/netapps/json/ccs/Release/" -i"C:/TI/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" -i"C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/ccs/free_rtos" -i"C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/ccs/Release" -i"C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/ccs/OS" --priority --reread_libs --diag_wrap=off --display_error_number --warn_sections --xml_link_info="airu-firmware_linkInfo.xml" --rom_model -o "airu-firmware.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

airu-firmware.bin: $(EXE_OUTPUTS)
	@echo 'Invoking: ARM Hex Utility'
	"C:/TI/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armhex"  -o "airu-firmware.bin" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(BIN_OUTPUTS__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "src\HDC1080.obj" "src\OPT3001.obj" "src\app_utils.obj" "src\device_status.obj" "src\diskio.obj" "src\ff.obj" "src\gpio_if.obj" "src\gps_if.obj" "src\hooks.obj" "src\i2c_if.obj" "src\internet_if.obj" "src\main.obj" "src\network_common.obj" "src\pinmux.obj" "src\pms_if.obj" "src\simplelink_if.obj" "src\smartconfig.obj" "src\startup_ccs.obj" "src\timer_if.obj" "src\uart_if.obj" 
	-$(RM) "src\HDC1080.d" "src\OPT3001.d" "src\app_utils.d" "src\device_status.d" "src\diskio.d" "src\ff.d" "src\gpio_if.d" "src\gps_if.d" "src\hooks.d" "src\i2c_if.d" "src\internet_if.d" "src\main.d" "src\network_common.d" "src\pinmux.d" "src\pms_if.d" "src\simplelink_if.d" "src\smartconfig.d" "src\startup_ccs.d" "src\timer_if.d" "src\uart_if.d" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-@echo 'convert to bin files'
	-"C:/TI/ccsv7/utils/tiobj2bin/tiobj2bin" "airu-firmware.out" "airu-firmware.bin" "C:/TI/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armofd" "C:/TI/ccsv7/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armhex" "C:/TI/ccsv7/utils/tiobj2bin/mkhex4bin"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

