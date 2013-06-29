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

#include <string.h>
#include <stddef.h>
#include "NsfMapper.h"


NsfMapper::NsfMapper(uint32_t numRomBanks)
{
    mCart = new unsigned char[(uint32_t)numRomBanks << 12];
    memset(mCart, 0, (uint32_t)numRomBanks << 12);
}

NsfMapper::~NsfMapper()
{
	delete [] mCart;
	mCart = NULL;
}


uint8_t NsfMapper::ReadByte_0000(uint16_t addr)
{
	// TODO: fill out
	return 0;
}

uint8_t NsfMapper::ReadByte_4000(uint16_t addr)
{
	// TODO: fill out
	return 0;
}


uint8_t NsfMapper::ReadByte_8000(uint16_t addr)
{
	// TODO: fill out
	return 0;
}


uint8_t NsfMapper::ReadByte_C000(uint16_t addr)
{
	// TODO: fill out
	return 0;
}


uint8_t NsfMapper::ReadByte(uint16_t addr)
{
	return (this->*mReadByteFunc[addr >> 12])(addr);
}

/*******************************************************************************************/

void NsfMapper::WriteByte_0000(uint16_t addr, uint8_t data)
{
}

void NsfMapper::WriteByte_4000(uint16_t addr, uint8_t data)
{
}

void NsfMapper::WriteByte_5000(uint16_t addr, uint8_t data)
{
	if (addr >= 0x5FF8 && addr <= 0x5FFF) {
		mRomTbl[addr - 0x5FF8] = mCart + data * 0x1000;
	}
}

void NsfMapper::WriteByte_8000(uint16_t addr, uint8_t data)
{
}

void NsfMapper::WriteByte_C000(uint16_t addr, uint8_t data)
{
}

void NsfMapper::WriteByte(uint16_t addr, uint8_t data)
{
	(this->*mWriteByteFunc[addr >> 12])(addr, data);
}


void NsfMapper::Reset()
{
	for (int i = 0; i < 8; i++) {
		mRomTbl[i] = NULL;
	}

	mReadByteFunc[0x00] = &NsfMapper::ReadByte_0000;
	mReadByteFunc[0x01] = &NsfMapper::ReadByte_0000;
	mReadByteFunc[0x02] = &NsfMapper::ReadByte_0000;
	mReadByteFunc[0x03] = &NsfMapper::ReadByte_0000;
	mReadByteFunc[0x04] = &NsfMapper::ReadByte_4000;
	mReadByteFunc[0x05] = &NsfMapper::ReadByte_4000;
	mReadByteFunc[0x06] = &NsfMapper::ReadByte_4000;
	mReadByteFunc[0x07] = &NsfMapper::ReadByte_4000;
	mReadByteFunc[0x08] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x09] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0A] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0B] = &NsfMapper::ReadByte_8000;
	mReadByteFunc[0x0C] = &NsfMapper::ReadByte_C000;
	mReadByteFunc[0x0D] = &NsfMapper::ReadByte_C000;
	mReadByteFunc[0x0E] = &NsfMapper::ReadByte_C000;
	mReadByteFunc[0x0F] = &NsfMapper::ReadByte_C000;

	mWriteByteFunc[0x00] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x01] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x02] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x03] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x04] = &NsfMapper::WriteByte_4000;
	mWriteByteFunc[0x05] = &NsfMapper::WriteByte_5000;
	mWriteByteFunc[0x06] = &NsfMapper::WriteByte_4000;
	mWriteByteFunc[0x07] = &NsfMapper::WriteByte_4000;
	mWriteByteFunc[0x08] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x09] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0A] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0B] = &NsfMapper::WriteByte_8000;
	mWriteByteFunc[0x0C] = &NsfMapper::WriteByte_C000;
	mWriteByteFunc[0x0D] = &NsfMapper::WriteByte_C000;
	mWriteByteFunc[0x0E] = &NsfMapper::WriteByte_C000;
	mWriteByteFunc[0x0F] = &NsfMapper::WriteByte_C000;
}
