################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/experiments.o \
../src/file_worker.o \
../src/hr_timer.o \
../src/test.o \
../src/test_env.o 

C_SRCS += \
../src/clock_gettime_mac.c \
../src/experiments.c \
../src/file_worker.c \
../src/hr_timer.c \
../src/test.c \
../src/test_env.c 

OBJS += \
./src/clock_gettime_mac.o \
./src/experiments.o \
./src/file_worker.o \
./src/hr_timer.o \
./src/test.o \
./src/test_env.o 

C_DEPS += \
./src/clock_gettime_mac.d \
./src/experiments.d \
./src/file_worker.d \
./src/hr_timer.d \
./src/test.d \
./src/test_env.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

