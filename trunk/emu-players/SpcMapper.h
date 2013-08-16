/*
 * SpcMapper.h
 *
 *  Created on: Jul 8, 2013
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

#ifndef SPCMAPPER_H_
#define SPCMAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"

class SDsp;
class SSmp;

class SpcMapper : public MemoryMapper
{
public:
	SpcMapper();
	virtual ~SpcMapper();

	virtual void Reset();
	virtual uint8_t ReadByte(uint16_t addr);
	virtual void WriteByte(uint16_t addr, uint8_t data);

	void SetCpu(SSmp *cpu) { mSSmp = cpu; }
	void SetDsp(SDsp *dsp) { mSDsp = dsp; }

	enum
	{
		R_TEST = 0xF0,
		R_CONTROL,
		R_DSPADDR,
		R_DSPDATA,
		R_CPUIO0,
		R_CPUIO1,
		R_CPUIO2,
		R_CPUIO3,
	};

	uint8_t *mRam;
	uint8_t mTest, mCtrl;
	uint8_t mDspAddr;
	SSmp *mSSmp;
	SDsp *mSDsp;
};

#endif /* SPCMAPPER_H_ */
