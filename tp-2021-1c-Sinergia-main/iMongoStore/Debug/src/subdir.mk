################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/archivos.c \
../src/iMongoStore.c \
../src/logger.c \
../src/movimientoBloques.c \
../src/sabotajes.c 

OBJS += \
./src/archivos.o \
./src/iMongoStore.o \
./src/logger.o \
./src/movimientoBloques.o \
./src/sabotajes.o 

C_DEPS += \
./src/archivos.d \
./src/iMongoStore.d \
./src/logger.d \
./src/movimientoBloques.d \
./src/sabotajes.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2021-1c-Sinergia/CommonsSinergicas/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


