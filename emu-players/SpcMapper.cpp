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


SpcMapper::SpcMapper()
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
	// TODO: handle special addresses
	return mRam[addr];
}

void SpcMapper::WriteByte(uint16_t addr, uint8_t data)
{
	// TODO: handle special addresses
	mRam[addr] = data;
}

