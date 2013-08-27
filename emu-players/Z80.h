/*
 * Z80.h
 *
 *  Created on: Aug 27, 2013
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

#ifndef Z80_H_
#define Z80_H_

#include <stdint.h>
#include "MemoryMapper.h"

class Z80
{
public:
	void Reset();
	void Run(uint32_t maxCycles);
	void SetMapper(MemoryMapper *mapper) { mMemory = mapper; }

	enum {
		FLAG_C = 0x01,
		FLAG_N = 0x02,
		FLAG_P = 0x04,
		FLAG_Y = 0x08,	// bit 3 of the result byte
		FLAG_H = 0x10,
		FLAG_X = 0x20,	// bit 5 of the result byte
		FLAG_Z = 0x40,
		FLAG_S = 0x80,
	};

	struct  __attribute__ ((__packed__)) {
		uint8_t F,A, C,B, E,D, L,H;
		uint16_t SP, PC;
	} mRegs;

	bool mHalted;
	uint32_t mCycles;

private:
	MemoryMapper *mMemory;
};

#endif
