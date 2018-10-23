################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../startup/startup_mke18f16.cpp 

OBJS += \
./startup/startup_mke18f16.o 

CPP_DEPS += \
./startup/startup_mke18f16.d 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -std=gnu++14 -D__NEWLIB__ -DSDK_OS_BAREMETAL -DFSL_RTOS_BM -DSDK_DEBUGCONSOLE=1 -DCPU_MKE18F512VLL16 -DCPU_MKE18F512VLL16_cm4 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I../board -I../source -I../ -I../drivers -I../CMSIS -I../utilities -I../startup -I"/Users/ned/Documents/MCUXpressoIDE_10.2.1/workspace/MKE18F512xxx16_Project/source/emulator" -O2 -fno-common -g3 -Wall -c -fmessage-length=0 -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


