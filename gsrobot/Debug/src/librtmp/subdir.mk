################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/librtmp/amf.o \
../src/librtmp/hashswf.o \
../src/librtmp/log.o \
../src/librtmp/parseurl.o \
../src/librtmp/rtmp.o 

C_SRCS += \
../src/librtmp/amf.c \
../src/librtmp/hashswf.c \
../src/librtmp/log.c \
../src/librtmp/parseurl.c \
../src/librtmp/rtmp.c 

OBJS += \
./src/librtmp/amf.o \
./src/librtmp/hashswf.o \
./src/librtmp/log.o \
./src/librtmp/parseurl.o \
./src/librtmp/rtmp.o 

C_DEPS += \
./src/librtmp/amf.d \
./src/librtmp/hashswf.d \
./src/librtmp/log.d \
./src/librtmp/parseurl.d \
./src/librtmp/rtmp.d 


# Each subdirectory must supply rules for building sources it contributes
src/librtmp/%.o: ../src/librtmp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


