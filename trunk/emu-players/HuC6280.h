/*
 * HuC6280.h
 *
 *  Created on: Aug 30, 2013
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

#ifndef HUC62802_H_
#define HUC62802_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "Oscillator.h"


class HuC6280PsgChannel : public Oscillator
{
public:
	virtual ~HuC6280PsgChannel() {}

	virtual void Reset();
	virtual void Step();
};


class HuC6280Psg
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	HuC6280PsgChannel mChannels[6];
};


class HuC6280
{
public:
	void Reset();
	void Run(uint32_t maxCycles);
	void SetMapper(MemoryMapper *mapper) { mMemory = mapper; }

	enum {
		FLAG_C = 0x01,
		FLAG_Z = 0x02,
		FLAG_I = 0x04,
		FLAG_D = 0x08,
		FLAG_T = 0x10,
		FLAG_V = 0x40,
		FLAG_N = 0x80,
	};

	struct {
		uint8_t A, X, Y, S, F;
		uint16_t PC;
	} mRegs;

	uint32_t mCycles;
	uint8_t mSpeed;
private:
	MemoryMapper *mMemory;
};

#endif

