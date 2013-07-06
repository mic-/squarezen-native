/*
 * NsfMapper.h
 *
 *  Created on: Jun 1, 2013
 *
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

#ifndef NSFMAPPER_H_
#define NSFMAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "Emu2A03.h"


class NsfMapper : public MemoryMapper
{
public:
	NsfMapper(uint32_t numRomBanks);
	virtual ~NsfMapper();

	virtual void Reset();
	virtual uint8_t ReadByte(uint16_t addr);
	virtual void WriteByte(uint16_t addr, uint8_t data);

	uint8_t *GetRomPointer() const { return mCart; }
	void SetApu(Emu2A03 *apu) { mApu = apu; }

private:
	uint8_t ReadByte_0000(uint16_t addr);
	uint8_t ReadByte_4000(uint16_t addr);
	uint8_t ReadByte_6000(uint16_t addr);
	uint8_t ReadByte_8000(uint16_t addr);

	void WriteByte_0000(uint16_t addr, uint8_t data);
	void WriteByte_4000(uint16_t addr, uint8_t data);
	void WriteByte_5000(uint16_t addr, uint8_t data);
	void WriteByte_6000(uint16_t addr, uint8_t data);
	void WriteByte_8000(uint16_t addr, uint8_t data);

	typedef uint8_t (NsfMapper::*ReadByteFunc)(uint16_t);
	typedef void (NsfMapper::*WriteByteFunc)(uint16_t, uint8_t);
	ReadByteFunc mReadByteFunc[0x10];
	WriteByteFunc mWriteByteFunc[0x10];

	uint8_t *mRomTbl[8];
	uint8_t *mRam;
	uint8_t *mExRam;
	uint8_t *mCart;
	Emu2A03 *mApu;
	uint16_t mNumRomBanks;
	uint8_t mCallCode[0x10];
};

#endif /* NSFMAPPER_H_ */
