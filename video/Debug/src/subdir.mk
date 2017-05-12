################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/\ RGB2YUV.cpp \
../src/video.cpp 

OBJS += \
./src/\ RGB2YUV.o \
./src/video.o 

CPP_DEPS += \
./src/\ RGB2YUV.d \
./src/video.d 


# Each subdirectory must supply rules for building sources it contributes
src/\ RGB2YUV.o: ../src/\ RGB2YUV.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/ RGB2YUV.d" -MT"src/\ RGB2YUV.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


