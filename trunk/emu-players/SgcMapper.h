/*
 * SgcMapper.h
 *
 *  Created on: Aug 29, 2013
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

#ifndef SGCMAPPER_H_
#define SGCMAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "Z80.h"
#include "SN76489.h"


class SgcMapper : public MemoryMapper
{
public:
	SgcMapper(uint32_t numRomPages);
	virtual ~SgcMapper();

	void SetPsg(SnChip *psg) { mPsg = psg; }
	void SetSystemType(uint8_t systemType);

	virtual void Reset();
	virtual uint8_t ReadByte(uint16_t addr);
	virtual void WriteByte(uint16_t addr, uint8_t data);
	virtual uint8_t ReadPort(uint16_t port);
	virtual void WritePort(uint16_t port, uint8_t addr);

	enum {
		FRAME2_CTRL = 0xFFFC,
		FRAME0_PAGE = 0xFFFD,
		FRAME1_PAGE = 0xFFFE,
		FRAME2_PAGE = 0xFFFF,
	};

	enum {
		FRAME2_AS_ROM = 0,
		FRAME2_AS_RAM = 8,
	};

private:
	uint8_t ReadByteSMSGG(uint16_t addr);
	uint8_t ReadByteCV(uint16_t addr);

	uint8_t ReadPortSMSGG(uint16_t port);
	uint8_t ReadPortCV(uint16_t port);

	void WriteByteSMSGG(uint16_t addr, uint8_t data);
	void WriteByteCV(uint16_t addr, uint8_t data);

	void WritePortSMSGG(uint16_t port, uint8_t data);
	void WritePortCV(uint16_t port, uint8_t data);

	typedef uint8_t (SgcMapper::*ReadFunc)(uint16_t);
	typedef void (SgcMapper::*WriteFunc)(uint16_t, uint8_t);
	ReadFunc mReadByteFunc;
	WriteFunc mWriteByteFunc;
	ReadFunc mReadPortFunc;
	WriteFunc mWritePortFunc;

	SnChip *mPsg;
	uint8_t *mCart;
	uint8_t *mRomTbl[3];
	uint8_t *mRam;
	uint8_t *mExRam;
	uint16_t mNumRomPages;
	uint8_t mSystemType;
	uint8_t mMapperRegs[4];
};

#endif	/* SGCMAPPER_H_ */
