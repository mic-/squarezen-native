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
#include "SndhMapper.h"


SndhMapper::SndhMapper(uint32_t fileSize)
{
	// ToDo: implement
	mFileImage = new uint8_t[fileSize];
	mRam = new uint8_t[0x80000];		// emulate a 512 kB RAM system
}

SndhMapper::~SndhMapper()
{
	delete [] mFileImage;
	mFileImage = NULL;
}


void SndhMapper::Reset()
{
	// ToDo: implement
	mWriteByteFunc[0x00] = &SndhMapper::WriteByte_00;
	mWriteByteFunc[0xFF] = &SndhMapper::WriteByte_FF;
}


uint8_t SndhMapper::ReadByte(uint32_t addr)
{
	// ToDo: implement
	return 0;
}

/* 0x00000000 - 0x00FFFFFF */
void SndhMapper::WriteByte_00(uint32_t addr, uint8_t data)
{
	if (addr <= SndhMapper::SUPERVISOR_RAM_END) {
		// ToDo: handle
	} else if (addr <= OS_BSS_RAM_END) {
		// ToDo: handle
	} else if (addr <= 0x7FFFF) {
		mRam[addr] = data;
	}
}


/* 0xFF000000 - 0xFFFFFFFF */
void SndhMapper::WriteByte_FF(uint32_t addr, uint8_t data)
{
	switch (addr) {
	case YM_ADDRESS:
		mYmAddressLatch = data;
		break;
	case YM_DATA:
		mYm->Write(mYmAddressLatch, data);
		break;
	}
}


void SndhMapper::WriteByte(uint32_t addr, uint8_t data)
{
	(this->*mWriteByteFunc[addr >> 12])(addr, data);
}
