################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/discordiador.c \
../src/logger.c \
../src/tripulante.c 

OBJS += \
./src/discordiador.o \
./src/logger.o \
./src/tripulante.o 

C_DEPS += \
./src/discordiador.d \
./src/logger.d \
./src/tripulante.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2021-1c-Sinergia/CommonsSinergicas/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


