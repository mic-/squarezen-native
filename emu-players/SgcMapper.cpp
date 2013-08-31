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
#include "SgcMapper.h"
#include "SgcPlayer.h"


SgcMapper::SgcMapper(uint32_t numRomPages)
	: mRam(NULL)
	, mExRam(NULL)
	, mNumRomPages(NULL)
{
    mCart = new uint8_t[(uint32_t)numRomPages << 14];
    memset(mCart, 0, (uint32_t)numRomPages << 14);
	mRam = new uint8_t[0x2000];
	mExRam = new uint8_t[0x4000];
	SetSystemType(SgcPlayer::SYSTEM_SMS);
}

SgcMapper::~SgcMapper()
{
	// ToDo: implement
	delete [] mCart;
	delete [] mRam;
	delete [] mExRam;

	mCart  = NULL;
	mRam   = NULL;
	mExRam = NULL;
}


void SgcMapper::SetSystemType(uint8_t systemType)
{
	NLOGV("SgcMapper", "SetSystemType(%d)", systemType);

	mSystemType = systemType;
	if (systemType <= SgcPlayer::SYSTEM_GG) {
		mReadByteFunc = &SgcMapper::ReadByteSMSGG;
		mReadPortFunc = &SgcMapper::ReadPortSMSGG;
		mWriteByteFunc = &SgcMapper::WriteByteSMSGG;
		mWritePortFunc = &SgcMapper::WritePortSMSGG;
	} else if (systemType == SgcPlayer::SYSTEM_CV) {
		mReadByteFunc = &SgcMapper::ReadByteCV;
		mReadPortFunc = &SgcMapper::ReadPortCV;
		mWriteByteFunc = &SgcMapper::WriteByteCV;
		mWritePortFunc = &SgcMapper::WritePortCV;
	} else {
		NLOGE("SgcMapper", "Invalid system type: %d", systemType);
	}
}


void SgcMapper::Reset()
{
	mRomTbl[0] = mCart;
	mRomTbl[1] = mCart + (1 % mNumRomPages) * 0x4000;
	mRomTbl[2] = mCart + (2 % mNumRomPages) * 0x4000;
}


uint8_t SgcMapper::ReadByteSMSGG(uint16_t addr)
{
	if (addr >= 0x4000 && addr <= 0x7FFF) {
		return mRomTbl[1][addr - 0x4000];

	} else if (addr >= 0x8000 && addr <= 0xBFFF) {
		if (mMapperRegs[FRAME2_CTRL & 3] & FRAME2_AS_RAM) {
			return mExRam[addr & 0x3FFF];
		} else {
			return mRomTbl[2][addr - 0x8000];
		}

	} else if (addr >= 0xC000) {
		if (addr >= FRAME2_CTRL) {
			return mMapperRegs[addr - FRAME2_CTRL];
		}
		return mRam[addr & 0x1FFF];
	}
	return 0;
}

uint8_t SgcMapper::ReadByteCV(uint16_t addr)
{
	// ToDo: implement
	if (addr >= 0x6000 && addr <= 0x7FFF) {
		// 1 kB of RAM, mirrored 8 times
		return mRam[addr & 0x3FF];

	} else if (addr >= 0x8000) {
		return mCart[addr - 0x8000];
	}

	return 0;
}

uint8_t SgcMapper::ReadByte(uint16_t addr)
{
	return (this->*mReadByteFunc)(addr);
}


void SgcMapper::WriteByteSMSGG(uint16_t addr, uint8_t data)
{
	// ToDo: implement
	if (addr >= 0x8000 && addr <= 0xBFFF) {
		if (mMapperRegs[FRAME2_CTRL & 3] & FRAME2_AS_RAM) {
			mExRam[addr & 0x3FFF] = data;
		}

	} else if (addr >= 0xC000) {
		// 8 kB of RAM, mirrored once
		mRam[addr & 0x1FFF] = data;
		if (addr >= FRAME2_CTRL) {
			mMapperRegs[addr - FRAME2_CTRL] = data;
		}
	}
}

void SgcMapper::WriteByteCV(uint16_t addr, uint8_t data)
{
	// ToDo: implement
	if (addr >= 0x6000 && addr <= 0x7FFF) {
		// 1 kB of RAM, mirrored 8 times
		mRam[addr & 0x3FF] = data;
	}
}

void SgcMapper::WriteByte(uint16_t addr, uint8_t data)
{
	(this->*mWriteByteFunc)(addr, data);
}


uint8_t SgcMapper::ReadPortSMSGG(uint16_t port)
{
	// ToDo: implement
	return 0;
}

uint8_t SgcMapper::ReadPortCV(uint16_t port)
{
	// ToDo: implement
	return 0;
}

uint8_t SgcMapper::ReadPort(uint16_t addr)
{
	return (this->*mReadPortFunc)(addr);
}


void SgcMapper::WritePortSMSGG(uint16_t addr, uint8_t data)
{
	// ToDo: implement
	if (addr >= 0x40 && addr <= 0x7F) {
		// SMS/GG uses ports 0x40-0x7F to write to the PSG
		mPsg->Write(0x7F, data);
	}
}

void SgcMapper::WritePortCV(uint16_t addr, uint8_t data)
{
	// ToDo: implement
	if (addr >= 0xE0 && addr <= 0xFF) {
		// ColecoVision uses ports 0xE0-0xFF to write to the PSG
		mPsg->Write(0x7F, data);
	}
}

void SgcMapper::WritePort(uint16_t addr, uint8_t data)
{
	(this->*mWritePortFunc)(addr, data);
}

