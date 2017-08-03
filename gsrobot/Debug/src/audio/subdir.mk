################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/audio/AudioCap.o \
../src/audio/AudioCapBase.o \
../src/audio/AudioCap_AAC.o \
../src/audio/AudioCap_File.o \
../src/audio/AudioCap_G711.o \
../src/audio/AudioCfg.o \
../src/audio/Audio_WAV2AAC.o 

CPP_SRCS += \
../src/audio/AudioCap.cpp \
../src/audio/AudioCapBase.cpp \
../src/audio/AudioCap_AAC.cpp \
../src/audio/AudioCap_File.cpp \
../src/audio/AudioCap_G711.cpp \
../src/audio/AudioCfg.cpp \
../src/audio/Audio_WAV2AAC.cpp 

OBJS += \
./src/audio/AudioCap.o \
./src/audio/AudioCapBase.o \
./src/audio/AudioCap_AAC.o \
./src/audio/AudioCap_File.o \
./src/audio/AudioCap_G711.o \
./src/audio/AudioCfg.o \
./src/audio/Audio_WAV2AAC.o 

CPP_DEPS += \
./src/audio/AudioCap.d \
./src/audio/AudioCapBase.d \
./src/audio/AudioCap_AAC.d \
./src/audio/AudioCap_File.d \
./src/audio/AudioCap_G711.d \
./src/audio/AudioCfg.d \
./src/audio/Audio_WAV2AAC.d 


# Each subdirectory must supply rules for building sources it contributes
src/audio/%.o: ../src/audio/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


