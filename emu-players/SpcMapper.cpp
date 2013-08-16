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
#include "SpcMapper.h"
#include "SDsp.h"
#include "SSmp.h"


SpcMapper::SpcMapper()
	: mTest(0x0A)
	, mCtrl(0xB0)
	, mSDsp(NULL)
{
	// TODO: implement
	mRam = new uint8_t[64 * 1024];
}

SpcMapper::~SpcMapper()
{
	delete [] mRam;
	mRam = NULL;
}


void SpcMapper::Reset()
{
	// TODO: implement
}


uint8_t SpcMapper::ReadByte(uint16_t addr)
{
	if (addr >= 0xFFC0 && (mCtrl & 0x80)) {
		// ToDo: reference IPL ROM
	}
	return mRam[addr];
}


void SpcMapper::WriteByte(uint16_t addr, uint8_t data)
{
	if (addr < 0xFFC0 || (mCtrl & 0x80) == 0) {
		mRam[addr] = data;
	}

	switch (addr) {
	case R_TEST:
		if (data & 4) {
			// "Crash" the CPU
			mSSmp->mRegs.PC ^= 0xAA55;
			mSSmp->mRegs.SP ^= 0xAA;
			mSSmp->mHalted = true;
		}
		break;

	case R_CONTROL:
		mCtrl = data;
		break;

	case R_DSPDATA:
		mSDsp->Write(mRam[R_DSPADDR], data);
		break;

	default:
		break;
	}
}

