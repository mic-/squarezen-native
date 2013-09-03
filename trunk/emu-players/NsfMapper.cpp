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

#include <string.h>
#include <stddef.h>
#include "NativeLogger.h"
#include "NsfMapper.h"
#include "KonamiVrc6.h"
#include "Namco163.h"
#include "Sunsoft5B.h"


NsfMapper::NsfMapper(uint32_t numRomBanks)
	: mVrc6(NULL)
	, mN163(NULL)
	, mSunsoft5B(NULL)
	, mNumRomBanks(numRomBanks)
{
    mCart = new uint8_t[(uint32_t)numRomBanks << 12];
    memset(mCart, 0, (uint32_t)numRomBanks << 12);
    mRam = new uint8_t[0x800];
    mExRam = new uint8_t[0x2000];
}

NsfMapper::~NsfMapper()
{
	delete [] mCart;
	delete [] mRam;
	delete [] mExRam;

	mCart  = NULL;
	mRam   = NULL;
	mExRam = NULL;
}


uint8_t NsfMapper::ReadByte_0000(uint16_t addr)
{
	if (addr < 0x2000) {
		return mRam[addr & 0x7FF];
	}
	return 0;
}

uint8_t NsfMapper::ReadByte_4000(uint16_t addr)
{
	if (addr >= 0x4f80 && addr < 0x4f90) {
		return mCallCode[addr - 0x4f80];

	} else if (addr >= 0x5FF8 && addr <= 0x5FFF) {
		NLOGD("NsfMapper", "Reading from bankswitch register (%#x)", addr);

	} else if (addr <= 0x4017) {
		NLOGD("NsfMapper", "APU read (%#x)", addr);
	}
	return 0;
}

uint8_t NsfMapper::ReadByte_6000(uint16_t addr)
{
	return mExRam[addr - 0x6000];
}

uint8_t NsfMapper::ReadByte_8000(uint16_t addr)
{
	if (mRomTbl[(addr >> 12) - 8]) {
		return mRomTbl[(addr >> 12) - 8][addr & 0x0FFF];
	}
	return 0;
}


uint8_t NsfMapper::ReadByte(uint16_t addr)
{
	return (this->*mReadByteFunc[addr >> 12])(addr);
}

/*******************************************************************************************/

void NsfMapper::WriteByte_0000(uint16_t addr, uint8_t data)
{
	if (addr < 0x2000) {
		mRam[addr & 0x7FF] = data;
	}
}

void NsfMapper::WriteByte_4000(uint16_t addr, uint8_t data)
{
	if (addr <= 0x4017) {
		//NLOGV("NsfMapper", "APU write (%#x, %#x)", addr, data);
		mApu->Write(addr, data);
	} else if (addr == 0x4800 && mN163) {	// Namco163 data port
		mN163->Write(mNamco163AddressLatch & 0x7F, data);
		if (mNamco163AddressLatch & 0x80) {	// auto-increment mode
			mNamco163AddressLatch = ((mNamco163AddressLatch + 1) & 0x7F) | 0x80;
		}
	} else 	if (addr >= 0x4f80 && addr < 0x4f90) {
		mCallCode[addr - 0x4f80] = data;
	}

}

void NsfMapper::WriteByte_5000(uint16_t addr, uint8_t data)
{
	if (addr >= 0x5FF8 && addr <= 0x5FFF) {
		if (data >= mNumRomBanks) {
			mRomTbl[addr - 0x5FF8] = NULL;
			NLOGV("NsfMapper", "Mapping nothing to #%x", 0x8000 + (addr - 0x5FF8) * 0x1000);
		} else {
			mRomTbl[addr - 0x5FF8] = mCart + data * 0x1000;
			NLOGV("NsfMapper", "Mapping bank %d to #%x", (data % mNumRomBanks), 0x8000 + (addr - 0x5FF8) * 0x1000);
		}
	}
}

void NsfMapper::WriteByte_6000(uint16_t addr, uint8_t data)
{
	mExRam[addr - 0x6000] = data;
}

void NsfMapper::WriteByte_8000(uint16_t addr, uint8_t data)
{
	switch (addr) {
	case 0x9000:	// VRC6 pulse1 control (MDDDVVVV)
	case 0x9001:	// VRC6 pulse1 freq low (LLLLLLLL)
	case 0x9002:	// VRC6 pulse1 freq high (E...HHHH)
	case 0xA000:	// VRC6 pulse2 control
	case 0xA001:	// VRC6 pulse2 freq low
	case 0xA002:	// VRC6 pulse2 freq high
	case 0xB000:	// VRC6 saw accumulator rate
	case 0xB001:	// VRC6 saw freq low
	case 0xB002:	// VRC6 saw freq high
		if (mVrc6) {
			mVrc6->Write(addr, data);
		}
		break;
	case 0xC000:	// Sunsoft5B address port
		if (mSunsoft5B) {
			mSunsoft5BAddressLatch = data;
		}
		break;
	case 0xE000:	// Sonsoft5B data port
		if (mSunsoft5B) {
			mSunsoft5B->Write(mSunsoft5BAddressLatch, data);
		}
		break;
	case 0xF800:	// Namco163 address port
		if (mN163) {
			mNamco163AddressLatch = data;
		}
		break;
	default:
		break;
	}
}


void NsfMapper::WriteByte(uint16_t addr, uint8_t data)
{
	(this->*mWriteByteFunc[addr >> 12])(addr, data);
}


void NsfMapper::Reset()
{
	memset(mRam, 0, 0x800);
	memset(mExRam, 0, 0x2000);

	for (int i = 0; i < 8; i++) {
		mRomTbl[i] = NULL;
	}

	mReadByteFunc[0x00] = &NsfMapper::ReadByte_0000;
	mReadByteFunc[0x01] = &NsfMapper::ReadByte_0000;
	mReadByteFunc[0x02] = &NsfMapper::ReadByte_0000;
	mReadByteFunc[0x03] = &NsfMapper::ReadByte_0000;

	mReadByteFunc[0x04] = &NsfMapper::ReadByte_4000;
	mReadByteFunc[0x05] = &NsfMapper::ReadByte_4000;
	mReadByteFunc[0x06] = &NsfMapper::ReadByte_6000;
	mReadByteFunc[0x07] = &NsfMapper::ReadByte_6000;

	mReadByteFunc[0x08] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x09] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0A] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0B] = &NsfMapper::ReadByte_8000;

	mReadByteFunc[0x0C] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0D] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0E] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0F] = &NsfMapper::ReadByte_8000;

	mWriteByteFunc[0x00] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x01] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x02] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x03] = &NsfMapper::WriteByte_0000;

	mWriteByteFunc[0x04] = &NsfMapper::WriteByte_4000;
	mWriteByteFunc[0x05] = &NsfMapper::WriteByte_5000;
	mWriteByteFunc[0x06] = &NsfMapper::WriteByte_6000;
	mWriteByteFunc[0x07] = &NsfMapper::WriteByte_6000;

	mWriteByteFunc[0x08] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x09] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0A] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0B] = &NsfMapper::WriteByte_8000;

	mWriteByteFunc[0x0C] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0D] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0E] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0F] = &NsfMapper::WriteByte_8000;
}
