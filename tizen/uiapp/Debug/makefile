################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk
CPP_DEPS += ./pch/tizenx.h.pch.d

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Squarezen.exe secondary-outputs

# Tool invocations
Squarezen.exe: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: C++ Linker'
	clang++.exe -o"Squarezen.exe" $(OBJS) $(USER_OBJS) $(LIBS) -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Xlinker --as-needed -pie -lpthread -Xlinker -rpath="/opt/usr/apps/0sM1rnfdHV/lib" -Xlinker -rpath="/home/developer/sdk_tools/lib" --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -L"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/lib" -L"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/lib/osp" -losp-uifw -losp-appfw -losp-image -losp-json -losp-ime -losp-net -lpthread -losp-content -losp-locations -losp-telephony -losp-uix -losp-media -losp-messaging -losp-web -losp-social -losp-wifi -losp-bluetooth -losp-nfc -losp-face -losp-speech-tts -losp-speech-stt -losp-shell -losp-shell-core -losp-vision -lxml2
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) ./pch/tizenx.h.pch.d ./pch/tizenx.h.pch
	-$(RM) $(MO_FILES)$(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(EDJ_FILES)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) Squarezen.exe
	-@echo ' '

secondary-outputs: $(MO_FILES) $(EDJ_FILES)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

$(OBJS):%.o:pch/tizenx.h.pch

pch/tizenx.h.pch: ../inc/tizenx.h ../.cproject ../.project
	@echo 'Building file: $<'
	@echo 'Generate pch file'
	clang++.exe -I"pch" -D_DEBUG -IC:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2 -IC:/tizen-sdk/library -IC:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include -IC:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -I"C:\Users\Michael\sqzworkspace\tizen\uiapp\inc" -O0 -g3 -Wall -c -fmessage-length=0 -target i386-tizen-linux-gnueabi -gcc-toolchain "C:/tizen-sdk/tools/smart-build-interface/../i386-linux-gnueabi-gcc-4.5/" -ccc-gcc-name i386-linux-gnueabi-g++ -march=i386 -Wno-gnu -fPIE --sysroot="C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/libxml2" -I"C:\tizen-sdk\library" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include" -I"C:/tizen-sdk/platforms/tizen2.1/rootstraps/tizen-emulator-2.1.native/usr/include/osp" -D_APP_LOG -MMD -MP -MF"./pch/tizenx.h.pch.d" -MT"./pch/tizenx.h.pch.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

