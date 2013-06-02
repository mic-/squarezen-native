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

#include "NsfMapper.h"

uint8_t NsfMapper::ReadByte(uint16_t addr)
{
	// TODO: fill out
	return 0;
}


void NsfMapper::WriteByte_0000(uint16_t addr, uint8_t data)
{
}

void NsfMapper::WriteByte_5000(uint16_t addr, uint8_t data)
{
	if (addr >= 0x5FF8 && addr <= 0x5FFF) {
		mRomTbl[addr - 0x5FF8] = mCart + data * 0x1000;
	}
}


void NsfMapper::WriteByte(uint16_t addr, uint8_t data)
{
	(this->*mWriteByteFunc[addr >> 12])(addr, data);
}


void NsfMapper::Reset()
{
	mWriteByteFunc[0x00] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x05] = &NsfMapper::WriteByte_5000;
}
