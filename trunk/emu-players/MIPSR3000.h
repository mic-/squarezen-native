/*
 * MIPSR3000.h
 *
 *  Created on: Feb 8, 2015
 *
 * Copyright 2015 Mic
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

#pragma once

#include <stdint.h>
#include "CpuBase.h"
#include "MemoryMapper.h"

class MIPSR3000 : public CpuBase
{
public:
	uint32_t mPC;
	uint32_t mRegs[32];
	uint32_t mCycles;
	uint32_t mHi, mLo;

	virtual void Reset();
	virtual void Run(uint32_t maxCycles);

	typedef void (MIPSR3000::*InstructionDecoder)(uint32_t);

	void RTypeAdd(uint32_t instruction);
	void RTypeAddu(uint32_t instruction);
	void RTypeAnd(uint32_t instruction);


private:
};

