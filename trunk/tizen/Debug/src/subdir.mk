################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/AppResourceId.cpp \
../src/Squarezen.cpp \
../src/SquarezenEntry.cpp \
../src/SquarezenFormFactory.cpp \
../src/SquarezenFrame.cpp \
../src/SquarezenMainForm.cpp \
../src/SquarezenPanelFactory.cpp 

OBJS += \
./src/AppResourceId.o \
./src/Squarezen.o \
./src/SquarezenEntry.o \
./src/SquarezenFormFactory.o \
./src/SquarezenFrame.o \
./src/SquarezenMainForm.o \
./src/SquarezenPanelFactory.o 

CPP_DEPS += \
./src/AppResourceId.d \
./src/Squarezen.d \
./src/SquarezenEntry.d \
./src/SquarezenFormFactory.d \
./src/SquarezenFrame.d \
./src/SquarezenMainForm.d \
./src/SquarezenPanelFactory.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


