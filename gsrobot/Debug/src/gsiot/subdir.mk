################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/gsiot/GSIOTClient.cpp \
../src/gsiot/GSIOTConfig.cpp \
../src/gsiot/GSIOTControl.cpp \
../src/gsiot/GSIOTDevice.cpp \
../src/gsiot/GSIOTDeviceInfo.cpp \
../src/gsiot/GSIOTEvent.cpp \
../src/gsiot/GSIOTHeartbeat.cpp \
../src/gsiot/GSIOTInfo.cpp \
../src/gsiot/GSIOTUser.cpp \
../src/gsiot/GSIOTUserMgr.cpp 

OBJS += \
./src/gsiot/GSIOTClient.o \
./src/gsiot/GSIOTConfig.o \
./src/gsiot/GSIOTControl.o \
./src/gsiot/GSIOTDevice.o \
./src/gsiot/GSIOTDeviceInfo.o \
./src/gsiot/GSIOTEvent.o \
./src/gsiot/GSIOTHeartbeat.o \
./src/gsiot/GSIOTInfo.o \
./src/gsiot/GSIOTUser.o \
./src/gsiot/GSIOTUserMgr.o 

CPP_DEPS += \
./src/gsiot/GSIOTClient.d \
./src/gsiot/GSIOTConfig.d \
./src/gsiot/GSIOTControl.d \
./src/gsiot/GSIOTDevice.d \
./src/gsiot/GSIOTDeviceInfo.d \
./src/gsiot/GSIOTEvent.d \
./src/gsiot/GSIOTHeartbeat.d \
./src/gsiot/GSIOTInfo.d \
./src/gsiot/GSIOTUser.d \
./src/gsiot/GSIOTUserMgr.d 


# Each subdirectory must supply rules for building sources it contributes
src/gsiot/%.o: ../src/gsiot/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


