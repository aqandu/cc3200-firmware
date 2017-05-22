################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
src/HDC1080.obj: ../src/HDC1080.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/HDC1080.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/OPT3001.obj: ../src/OPT3001.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/OPT3001.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/PMS3003.obj: ../src/PMS3003.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/PMS3003.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/device_status.obj: ../src/device_status.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/device_status.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/diskio.obj: ../src/diskio.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/diskio.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ff.obj: ../src/ff.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/ff.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/gpio_if.obj: ../src/gpio_if.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/gpio_if.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/hooks.obj: ../src/hooks.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/hooks.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/i2c_if.obj: C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common/i2c_if.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/i2c_if.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/main.obj: ../src/main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/main.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/network_common.obj: C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common/network_common.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/network_common.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/pinmux.obj: ../src/pinmux.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/pinmux.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/simplelink_if.obj: ../src/simplelink_if.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/simplelink_if.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/smartconfig.obj: ../src/smartconfig.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/smartconfig.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/startup_ccs.obj: C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common/startup_ccs.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/startup_ccs.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/uart_if.obj: C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common/uart_if.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --opt_for_speed=5 --include_path="C:/TI/ccsv6/tools/compiler/ti-cgt-arm_16.9.1.LTS/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/include" --include_path="C:/Users/jonathan/workspace_v6_1_3/airu-firmware/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/example/common" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/inc" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink/source" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/driverlib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/oslib/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/include" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/third_party/FreeRTOS/source/portable/CCS/ARM_CM3" --include_path="C:/TI/CC3200SDK_1.3.0/cc3200-sdk/simplelink_extlib/provisioninglib" --define=ccs --define=SL_PLATFORM_MULTI_THREADED --define=USE_FREERTOS --define=cc3200 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/uart_if.d" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

