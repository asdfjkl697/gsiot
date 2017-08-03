################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/h264/cap_desc.c \
../src/h264/cap_desc_parser.c \
../src/h264/h264_xu_ctrls.c \
../src/h264/nalu.c \
../src/h264/v4l2uvc.c \
../src/h264/videoh264.c 

OBJS += \
./src/h264/cap_desc.o \
./src/h264/cap_desc_parser.o \
./src/h264/h264_xu_ctrls.o \
./src/h264/nalu.o \
./src/h264/v4l2uvc.o \
./src/h264/videoh264.o 

C_DEPS += \
./src/h264/cap_desc.d \
./src/h264/cap_desc_parser.d \
./src/h264/h264_xu_ctrls.d \
./src/h264/nalu.d \
./src/h264/v4l2uvc.d \
./src/h264/videoh264.d 


# Each subdirectory must supply rules for building sources it contributes
src/h264/%.o: ../src/h264/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


