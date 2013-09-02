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
#include "KssMapper.h"


KssMapper::KssMapper(uint32_t numRomBanks)
	: mAy(NULL)
	, mScc(NULL)
	, mSN76489(NULL)
{
	// ToDo: implement
}

KssMapper::~KssMapper()
{
	// ToDo: implement
}

void KssMapper::Reset()
{
	// ToDo: implement
}

uint8_t KssMapper::ReadByte(uint16_t addr)
{
	// ToDo: implement
	return 0;
}

void KssMapper::WriteByte(uint16_t addr, uint8_t data)
{
	// ToDo: implement
	switch (addr) {
	case SCC_ENABLE:
		// ToDo: activate SCC if bits 0..5 == 0x3F
		break;
	case SN_PORT:
	case SN_PORT_MIRROR:
		if (mSN76489) {
			mSN76489->Write(0x7F, data);
		}
		break;
	case AY_ADDRESS_PORT:
		mAyAddressLatch = data;
		break;
	case AY_DATA_PORT:
		if (mAy) {
			mAy->Write(mAyAddressLatch, data);
		}
		break;
	}
}
