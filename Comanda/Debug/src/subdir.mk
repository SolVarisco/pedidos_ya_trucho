################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/clock.c \
../src/comanda.c \
../src/lru.c \
../src/memory.c 

OBJS += \
./src/clock.o \
./src/comanda.o \
./src/lru.o \
./src/memory.o 

C_DEPS += \
./src/clock.d \
./src/comanda.d \
./src/lru.d \
./src/memory.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/tp-2020-2c-CabreadOS/Commons-cabronas/src -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


