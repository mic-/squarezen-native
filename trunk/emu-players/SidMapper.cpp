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

#define NLOG_LEVEL_WARNING 0

#include <string.h>
#include <stddef.h>
#include "NativeLogger.h"
#include "SidMapper.h"
#include "SidPlayer.h"


SidMapper::SidMapper()
	: mSid(NULL)
	, mSidPlayer(NULL)
{
	mRam = new uint8_t[64 * 1024];
	memset(mRam, 0, 64*1024);
}


SidMapper::~SidMapper()
{
	delete [] mRam;
	mRam = NULL;
}


uint8_t SidMapper::ReadByte(uint16_t addr)
{
	// TODO: handle special addresses?
	if (addr >= Mos6581::REGISTER_BASE && addr <= Mos6581::REGISTER_BASE + Mos6581::R_FILTER_MODEVOL) {
		NLOGD("SidMapper", "Reading from SID area (%#x)", addr);
	} else if ((addr >> 12) >= 0xE && (mRam[1] & 3) >= 2) {
		NLOGD("SidMapper", "Reading from kernel ROM (%#x)", addr);
	}
	return mRam[addr];
}


void SidMapper::WriteByte(uint16_t addr, uint8_t data)
{
	NLOGV("SidMapper", "Write(%#x, %#x)", addr, data);

	uint8_t bankSelect = mRam[1] & 7;

	switch (addr >> 12) {
	case 0xA: case 0xB:
		if ((bankSelect & 3) != 3) {
			mRam[addr] = data;
		}
		break;
	case 0xD:
		NLOGV("SidMapper", "addr=%#x, data=%#x, bankSelect=%d", addr, data, bankSelect);
		if (bankSelect >= 5) {
			if (addr >= 0xDC00 && addr < 0xDC10) {
				NLOGE("SidMapper", "Writing %#x to %#x", data, addr);
			}
			// I/O at D000-DFFF
			if (addr >= Mos6581::REGISTER_BASE && addr <= Mos6581::REGISTER_BASE + 0x3FF) {
				addr &= (Mos6581::REGISTER_BASE + 0x1F);
				mSid->Write(addr, data);
				if (addr == Mos6581::REGISTER_BASE + Mos6581::R_FILTER_MODEVOL) {
					mSidPlayer->SetMasterVolume(data & 0x0F);
				}
			}
		} else if (bankSelect == 4 || bankSelect == 0) {
			// RAM at D000-DFFF
			mRam[addr] = data;
			NLOGD("SidMapper", "D000-DFFF mapped to RAM");
		} else {
			NLOGD("SidMapper", "D000-DFFF mapped to ROM");
		}
		break;
	case 0xE: case 0xF:
		if ((bankSelect & 3) < 2) {
			mRam[addr] = data;
		}
		break;
	default:
		mRam[addr] = data;
		break;
	}

}


void SidMapper::Reset()
{
	mRam[1] = 0x36; //0x37;
}

