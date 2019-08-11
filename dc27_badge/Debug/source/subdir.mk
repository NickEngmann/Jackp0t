################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/dc27_badge.c \
../source/mtb.c \
../source/semihost_hardfault.c 

OBJS += \
./source/dc27_badge.o \
./source/mtb.o \
./source/semihost_hardfault.o 

C_DEPS += \
./source/dc27_badge.d \
./source/mtb.d \
./source/semihost_hardfault.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DSDK_OS_BAREMETAL -DFSL_RTOS_BM -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DCPU_MKL27Z64VDA4_cm0plus -DCPU_MKL27Z64VDA4 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I../board -I../utilities -I../drivers -I../source -I../ -I../startup -I../CMSIS -O1 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


