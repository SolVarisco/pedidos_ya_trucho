################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cocina.c \
../src/logger.c \
../src/planificacion.c \
../src/restaurante.c 

OBJS += \
./src/cocina.o \
./src/logger.o \
./src/planificacion.o \
./src/restaurante.o 

C_DEPS += \
./src/cocina.d \
./src/logger.d \
./src/planificacion.d \
./src/restaurante.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


