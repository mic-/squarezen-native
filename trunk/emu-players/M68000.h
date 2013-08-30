/*
 * M68000.h
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

#ifndef M68000_H_
#define M68000_H_

#include <stdint.h>
#include "MemoryMapper.h"

class M68000
{
public:
	void Reset();
	void Run(uint32_t maxCycles);

	enum {
		FLAG_C = 0x01,
		FLAG_V = 0x02,
		FLAG_Z = 0x04,
		FLAG_N = 0x08,
		FLAG_X = 0x10,
	};

	struct {
		uint32_t D[8];
		uint32_t A[8];
		uint32_t PC;
		uint8_t CCR;
	} mRegs;

	uint32_t mCycles;
};

#endif
