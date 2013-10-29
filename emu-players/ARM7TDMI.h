/*
 * ARM7TDMI.h
 *
 *  Created on: Oct 29, 2013
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

#ifndef ARM7TDMI_H_
#define ARM7TDMI_H_

#include <stdint.h>
#include "MemoryMapper.h"

class ARM7TDMI
{
public:
	void Reset();
	void Run(uint32_t maxCycles);
	void SetMapper(MemoryMapper *mapper) { mMemory = mapper; }

	uint32_t mCycles;
	MemoryMapper *mMemory;
	int32_t mRegs[16];
	int32_t mRegsUsr[16];
	int32_t mRegsFiq[16];
	int32_t mRegsSvc[16];
	int32_t mRegsAbt[16];
	int32_t mRegsIrq[16];
	int32_t mRegsUnd[16];
	int32_t *mRegBank;

	uint32_t 	mCpsr;
	uint32_t 	mSpsrFiq, mSpsrSvc, mSpsrAbt, mSpsrIrq, mSpsrUnd;
	uint32_t	*mSpsr;

	uint32_t	mFlags[9];
	uint32_t	mFlagsUsr[9];
	uint32_t	mFlagsIrq[9];
	uint32_t	*mCurFlags;

	typedef void (ARM7TDMI::*InstructionDecoder)(uint32_t);

	void ThumbType00(uint32_t instruction);

private:
	inline void DecodeARM(uint32_t instruction);
	inline void DecodeThumb(uint32_t instruction);

};

#endif	/* ARM7TDMI_H_ */

