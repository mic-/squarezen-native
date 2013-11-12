/*
 * Copyright 2013 Mic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define NLOG_LEVEL_VERBOSE 0
#define NLOG_TAG "SapMapper"

#include <string.h>
#include <stddef.h>
#include "NativeLogger.h"
#include "SapMapper.h"
#include "Pokey.h"


/*
0000-CFFF - RAM.
D000-D0FF - GTIA chip mirrored 8 times. ASAP maps just the PAL register at D014 for NTSC/PAL detection and the CONSOL register at D01F for 1-bit sounds, the rest is RAM.
D100-D1FF - reserved for parallel devices connected to the Atari, do not use.
D200-D2FF - POKEY chip mirrored 16 times or two POKEY chips (stereo) mirrored 8 times. At least the AUDF1-4, AUDC1-4 and AUDCTL registers must be implemented in a SAP player. Emulation of timer interrupts via IRQEN and IRQST is strongly recommended. SAP by Adam Bienias emulates only timer 1 interrupts.
D300-D3FF - reserved for the PIA chip, do not use.
D400-D4FF - ANTIC chip mirrored 16 times. SAP files may rely on WSYNC (D40A) and VCOUNT (D40B) registers. ASAP also implements NMIST/NMIRES (D40F), but not NMIEN (D40E). The playback routine should be driven by the PLAYER/FASTPLAY mechanism instead of directly programming ANTIC interrupts.
D500-D5FF - reserved for cartridge, do not use.
D600-D6FF - COVOX chip if enabled via the COVOX tag (ASAP only). The COVOX consists of four unsigned 8-bit DACs: 0 and 3 for the left channel, 1 and 2 for the right channel.
D700-D7FF - reserved for expansions, do not use.
D800-FFFF - RAM. FFFE/FFFF is 6502’s interrupt vector for POKEY timer interrupts.
*/

SapMapper::SapMapper(uint32_t numRomBanks)
{
	// ToDo: implement
}


SapMapper::~SapMapper()
{
	// ToDo: implement
}


uint8_t SapMapper::ReadByte(uint32_t addr)
{
	if (addr < 0xD000) {
		return mRam[addr];

	} else if (addr < 0xD100) {
		// ToDo: handle GTIA reads?

	} else if (addr >= 0xD200 && addr < 0xD300) {
		// ToDo: handle POKEY reads

	} else if (addr >= 0xD800) {
		return mRam[addr];
	}

	return 0;
}


void SapMapper::WriteByte(uint32_t addr, uint8_t data)
{
	if (addr < 0xD000) {
		mRam[addr] = data;

	} else if (addr < 0xD100) {
		// ToDo: handle GTIA writes?

	} else if (addr >= 0xD200 && addr < 0xD300) {
		mApu->Write(0xD200 | (addr & 0x0F), data);

	} else if (addr >= 0xD800) {
		mRam[addr] = data;
	}
}


void SapMapper::Reset()
{
	// ToDo: implement
}
