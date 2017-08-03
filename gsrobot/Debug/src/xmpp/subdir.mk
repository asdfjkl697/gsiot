################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/xmpp/XmppGSAuth.o \
../src/xmpp/XmppGSChange.o \
../src/xmpp/XmppGSEvent.o \
../src/xmpp/XmppGSManager.o \
../src/xmpp/XmppGSMessage.o \
../src/xmpp/XmppGSPlayback.o \
../src/xmpp/XmppGSPreset.o \
../src/xmpp/XmppGSRelation.o \
../src/xmpp/XmppGSReport.o \
../src/xmpp/XmppGSResult.o \
../src/xmpp/XmppGSState.o \
../src/xmpp/XmppGSTalk.o \
../src/xmpp/XmppGSUpdate.o \
../src/xmpp/XmppGSVObj.o \
../src/xmpp/XmppRegister.o 

CPP_SRCS += \
../src/xmpp/XmppGSAuth.cpp \
../src/xmpp/XmppGSChange.cpp \
../src/xmpp/XmppGSEvent.cpp \
../src/xmpp/XmppGSManager.cpp \
../src/xmpp/XmppGSMessage.cpp \
../src/xmpp/XmppGSPlayback.cpp \
../src/xmpp/XmppGSPreset.cpp \
../src/xmpp/XmppGSRelation.cpp \
../src/xmpp/XmppGSReport.cpp \
../src/xmpp/XmppGSResult.cpp \
../src/xmpp/XmppGSState.cpp \
../src/xmpp/XmppGSTalk.cpp \
../src/xmpp/XmppGSUpdate.cpp \
../src/xmpp/XmppGSVObj.cpp \
../src/xmpp/XmppRegister.cpp 

OBJS += \
./src/xmpp/XmppGSAuth.o \
./src/xmpp/XmppGSChange.o \
./src/xmpp/XmppGSEvent.o \
./src/xmpp/XmppGSManager.o \
./src/xmpp/XmppGSMessage.o \
./src/xmpp/XmppGSPlayback.o \
./src/xmpp/XmppGSPreset.o \
./src/xmpp/XmppGSRelation.o \
./src/xmpp/XmppGSReport.o \
./src/xmpp/XmppGSResult.o \
./src/xmpp/XmppGSState.o \
./src/xmpp/XmppGSTalk.o \
./src/xmpp/XmppGSUpdate.o \
./src/xmpp/XmppGSVObj.o \
./src/xmpp/XmppRegister.o 

CPP_DEPS += \
./src/xmpp/XmppGSAuth.d \
./src/xmpp/XmppGSChange.d \
./src/xmpp/XmppGSEvent.d \
./src/xmpp/XmppGSManager.d \
./src/xmpp/XmppGSMessage.d \
./src/xmpp/XmppGSPlayback.d \
./src/xmpp/XmppGSPreset.d \
./src/xmpp/XmppGSRelation.d \
./src/xmpp/XmppGSReport.d \
./src/xmpp/XmppGSResult.d \
./src/xmpp/XmppGSState.d \
./src/xmpp/XmppGSTalk.d \
./src/xmpp/XmppGSUpdate.d \
./src/xmpp/XmppGSVObj.d \
./src/xmpp/XmppRegister.d 


# Each subdirectory must supply rules for building sources it contributes
src/xmpp/%.o: ../src/xmpp/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


