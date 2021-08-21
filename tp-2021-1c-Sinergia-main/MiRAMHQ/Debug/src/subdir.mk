################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Memoria.c \
../src/gui.c \
../src/logger.c \
../src/miRAMHQ.c \
../src/paginacion.c \
../src/segmentacion.c 

OBJS += \
./src/Memoria.o \
./src/gui.o \
./src/logger.o \
./src/miRAMHQ.o \
./src/paginacion.o \
./src/segmentacion.o 

C_DEPS += \
./src/Memoria.d \
./src/gui.d \
./src/logger.d \
./src/miRAMHQ.d \
./src/paginacion.d \
./src/segmentacion.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2021-1c-Sinergia/CommonsSinergicas/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


