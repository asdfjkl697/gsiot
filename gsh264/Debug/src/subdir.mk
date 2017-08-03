################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cap_desc.c \
../src/cap_desc_parser.c \
../src/h264_xu_ctrls.c \
../src/nalu.c \
../src/v4l2uvc.c \
../src/video.c 

OBJS += \
./src/cap_desc.o \
./src/cap_desc_parser.o \
./src/h264_xu_ctrls.o \
./src/nalu.o \
./src/v4l2uvc.o \
./src/video.o 

C_DEPS += \
./src/cap_desc.d \
./src/cap_desc_parser.d \
./src/h264_xu_ctrls.d \
./src/nalu.d \
./src/v4l2uvc.d \
./src/video.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


