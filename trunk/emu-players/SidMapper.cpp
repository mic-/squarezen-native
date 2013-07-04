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
#include "SidMapper.h"


SidMapper::SidMapper()
{
	// TODO: implement
	mRam = new uint8_t[64 * 1024];
}

SidMapper::~SidMapper()
{
	// TODO: implement
	delete [] mRam;
	mRam = NULL;
}

uint8_t SidMapper::ReadByte(uint16_t addr)
{
	// TODO: handle special addresses?
	return mRam[addr];
}

void SidMapper::WriteByte(uint16_t addr, uint8_t data)
{
	mRam[addr] = data;
	if (addr >= Mos6581::REGISTER_BASE && addr <= Mos6581::REGISTER_BASE + Mos6581::R_FILTER_MODEVOL) {
		mSid->Write(addr, data);
	}
}


void SidMapper::Reset()
{
	// TODO: implement
}

