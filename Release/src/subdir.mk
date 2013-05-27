################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/AppResourceId.cpp \
../src/Blip_Buffer.cpp \
../src/SN76489.cpp \
../src/Squarezen.cpp \
../src/SquarezenEntry.cpp \
../src/SquarezenFormFactory.cpp \
../src/SquarezenFrame.cpp \
../src/SquarezenMainForm.cpp \
../src/SquarezenPanelFactory.cpp \
../src/VgmPlayer.cpp \
../src/YM2149.cpp \
../src/YmPlayer.cpp 

OBJS += \
./src/AppResourceId.o \
./src/Blip_Buffer.o \
./src/SN76489.o \
./src/Squarezen.o \
./src/SquarezenEntry.o \
./src/SquarezenFormFactory.o \
./src/SquarezenFrame.o \
./src/SquarezenMainForm.o \
./src/SquarezenPanelFactory.o \
./src/VgmPlayer.o \
./src/YM2149.o \
./src/YmPlayer.o 

CPP_DEPS += \
./src/AppResourceId.d \
./src/Blip_Buffer.d \
./src/SN76489.d \
./src/Squarezen.d \
./src/SquarezenEntry.d \
./src/SquarezenFormFactory.d \
./src/SquarezenFrame.d \
./src/SquarezenMainForm.d \
./src/SquarezenPanelFactory.d \
./src/VgmPlayer.d \
./src/YM2149.d \
./src/YmPlayer.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -I"C:\Users\Michael\workspace\tizen\Squarezen\inc" -O3 -g -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


