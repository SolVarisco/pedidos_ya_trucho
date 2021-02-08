################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Consola.c \
../src/Sindicato.c \
../src/afip.c \
../src/logger.c 

OBJS += \
./src/Consola.o \
./src/Sindicato.o \
./src/afip.o \
./src/logger.o 

C_DEPS += \
./src/Consola.d \
./src/Sindicato.d \
./src/afip.d \
./src/logger.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/tp-2020-2c-CabreadOS/Commons-cabronas/src -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


