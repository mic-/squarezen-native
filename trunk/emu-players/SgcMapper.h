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
	SgcMapper(uint32_t numRomBanks);
	virtual ~SgcMapper();

	void SetPsg(SnChip *psg) { mPsg = psg; }

	virtual void Reset();
	virtual uint8_t ReadByte(uint16_t addr);
	virtual void WriteByte(uint16_t addr, uint8_t data);
	virtual uint8_t ReadPort(uint16_t port);
	virtual void WritePort(uint16_t port, uint8_t addr);

private:
	SnChip *mPsg;
};

#endif
