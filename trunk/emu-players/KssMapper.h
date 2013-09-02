/*
 * KssMapper.h
 *
 *  Created on: Sep 2, 2013
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

#ifndef KSSMAPPER_H_
#define KSSMAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "KonamiScc.h"
#include "SN76489.h"
#include "YM2149.h"


class KssMapper : public MemoryMapper
{
public:
	KssMapper(uint32_t numRomBanks);
	virtual ~KssMapper();

	virtual void Reset();
	virtual uint8_t ReadByte(uint16_t addr);
	virtual void WriteByte(uint16_t addr, uint8_t data);

	void SetAy(YmChip *ay) { mAy = ay; }
	void SetScc(KonamiScc *scc) { mScc = scc; }
	void SetSN76489(SnChip *sn76489) { mSN76489 = sn76489; }

	enum {
		SN_PORT = 0x7E,
		SN_PORT_MIRROR = 0x7F,
		AY_ADDRESS_PORT = 0xA0,
		AY_DATA_PORT = 0xA1,
		SCC_ENABLE = 0x9000,
	};

private:
	YmChip *mAy;
	KonamiScc *mScc;
	SnChip *mSN76489;
	uint8_t mAyAddressLatch;
	bool mSccEnabled;
};

#endif
