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
#include "HesMapper.h"
#include "HuC6280.h"


HesMapper::HesMapper(uint32_t numRomPages)
	: mNumRomPages(numRomPages)
{
	// ToDo: implement
	mCart = new uint8_t[numRomPages * 0x2000];
	mRam = new uint8_t[0x2000];
}

HesMapper::~HesMapper()
{
	// ToDo: implement
	delete mCart;
	delete mRam;

	mCart = NULL;
	mRam  = NULL;
}

void HesMapper::Reset()
{
}

uint8_t HesMapper::ReadByte(uint32_t addr)
{
	uint32_t page = (addr >> 13) & 7;
	uint32_t mpr = mMPR[page];
	uint32_t offset = addr & 0x1FFF;

	if (mpr < 0x80) {
		if (mpr < mNumRomPages) {
			return mCart[mpr * 0x2000 + offset];
		}
	} else if (HuC6280Mapper::MPR_RAM_PAGE == mpr) {
		return mRam[offset];
	} else if (HuC6280Mapper::MPR_IO_PAGE == mpr) {
		// ToDo: handle IO reads
		if (HuC6280::R_TIMER_COUNT == offset) {
			return m6280->mTimer.mPos;
		}
	}

	return 0;
}

void HesMapper::WriteByte(uint32_t addr, uint8_t data)
{
	uint32_t page = (addr >> 13) & 7;
	uint32_t offset = addr & 0x1FFF;

	switch (mMPR[page]) {
	case HuC6280Mapper::MPR_RAM_PAGE:
		mRam[offset] = data;
		break;
	case HuC6280Mapper::MPR_IO_PAGE:
		if (offset >= HuC6280Psg::R_CHN_SELECT && offset <= HuC6280Psg::R_LFO_CTRL) {
			mPsg->Write(offset, data);
		} else if (HuC6280::R_TIMER_COUNT == offset) {
			m6280->mTimer.mPos = data & 0x7F;
		} else if (HuC6280::R_TIMER_CTRL == offset) {
			m6280->mTimer.mCtrl = data;
		}
		break;
	}
}

