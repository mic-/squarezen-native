/*
 * SidMapper.h
 *
 *  Created on: Jul 4, 2013
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

#ifndef SIDMAPPER_H_
#define SIDMAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "Mos6581.h"

class SidPlayer;

class SidMapper : public MemoryMapper
{
public:
	SidMapper();
	virtual ~SidMapper();

	uint8_t *GetRamPointer() const { return mRam; }
	void SetSid(Mos6581 *sid) { mSid = sid; }
	void SetSidPlayer(SidPlayer *sidPlayer) { mSidPlayer = sidPlayer; }

	virtual void Reset();
	virtual uint8_t ReadByte(uint16_t addr);
	virtual void WriteByte(uint16_t addr, uint8_t data);

private:
	uint8_t *mRam;
	Mos6581 *mSid;
	SidPlayer *mSidPlayer;
};


#endif
