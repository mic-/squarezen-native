################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
C:/Work/squarezen-native/emu-players/Blip_Buffer.cpp \
C:/Work/squarezen-native/emu-players/GbMemory.cpp \
C:/Work/squarezen-native/emu-players/GbPapu.cpp \
C:/Work/squarezen-native/emu-players/GbZ80.cpp \
C:/Work/squarezen-native/emu-players/GbsPlayer.cpp \
C:/Work/squarezen-native/emu-players/SN76489.cpp \
C:/Work/squarezen-native/emu-players/VgmPlayer.cpp \
C:/Work/squarezen-native/emu-players/YM2149.cpp \
C:/Work/squarezen-native/emu-players/YmPlayer.cpp 

OBJS += \
./emu-players/Blip_Buffer.o \
./emu-players/GbMemory.o \
./emu-players/GbPapu.o \
./emu-players/GbZ80.o \
./emu-players/GbsPlayer.o \
./emu-players/SN76489.o \
./emu-players/VgmPlayer.o \
./emu-players/YM2149.o \
./emu-players/YmPlayer.o 

CPP_DEPS += \
./emu-players/Blip_Buffer.d \
./emu-players/GbMemory.d \
./emu-players/GbPapu.d \
./emu-players/GbZ80.d \
./emu-players/GbsPlayer.d \
./emu-players/SN76489.d \
./emu-players/VgmPlayer.d \
./emu-players/YM2149.d \
./emu-players/YmPlayer.d 


# Each subdirectory must supply rules for building sources it contributes
emu-players/Blip_Buffer.o: C:/Work/squarezen-native/emu-players/Blip_Buffer.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/GbMemory.o: C:/Work/squarezen-native/emu-players/GbMemory.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/GbPapu.o: C:/Work/squarezen-native/emu-players/GbPapu.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/GbZ80.o: C:/Work/squarezen-native/emu-players/GbZ80.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/GbsPlayer.o: C:/Work/squarezen-native/emu-players/GbsPlayer.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/SN76489.o: C:/Work/squarezen-native/emu-players/SN76489.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/VgmPlayer.o: C:/Work/squarezen-native/emu-players/VgmPlayer.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/YM2149.o: C:/Work/squarezen-native/emu-players/YM2149.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

emu-players/YmPlayer.o: C:/Work/squarezen-native/emu-players/YmPlayer.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: C++ Compiler'
	clang++.exe -I"pch" -D_DEBUG -I"C:\Work\squarezen-native\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


