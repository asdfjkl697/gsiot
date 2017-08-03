################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/client.cpp \
../src/hardware.cpp \
../src/netpacket.cpp \
../src/obsolete.cpp \
../src/packet.cpp \
../src/radionetwork.cpp \
../src/radiopacket.cpp \
../src/radiosynchandler.cpp \
../src/taskmanager.cpp \
../src/tcpserver.cpp \
../src/udpserver.cpp 

C_SRCS += \
../src/rs232.c 

CC_SRCS += \
../src/main.cc 

OBJS += \
./src/client.o \
./src/hardware.o \
./src/main.o \
./src/netpacket.o \
./src/obsolete.o \
./src/packet.o \
./src/radionetwork.o \
./src/radiopacket.o \
./src/radiosynchandler.o \
./src/rs232.o \
./src/taskmanager.o \
./src/tcpserver.o \
./src/udpserver.o 

C_DEPS += \
./src/rs232.d 

CC_DEPS += \
./src/main.d 

CPP_DEPS += \
./src/client.d \
./src/hardware.d \
./src/netpacket.d \
./src/obsolete.d \
./src/packet.d \
./src/radionetwork.d \
./src/radiopacket.d \
./src/radiosynchandler.d \
./src/taskmanager.d \
./src/tcpserver.d \
./src/udpserver.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


