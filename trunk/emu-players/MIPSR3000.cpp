/*
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

#define NLOG_TAG "MIPSR3000"
#define NLOG_LEVEL_VERBOSE 0

#include <string>
#include <stdio.h>
#include "NativeLogger.h"
#include "MIPSR3000.h"

static uint32_t nextInstruction;

static const MIPSR3000::InstructionDecoder IJTYPE_DECODER_TABLE[] =
{

};

static const MIPSR3000::InstructionDecoder RTYPE_DECODER_TABLE[] =
{

};

#define FETCH_DBSI \
	nextInstruction = mMemory->ReadWord(mPC)

// Execute the instruction in the delayed branch slot
#define EXECUTE_DBSI \
	if (nextInstruction) { \
		if (nextInstruction & 0x0FC000000) { \
			(this->*IJTYPE_DECODER_TABLE[nextInstruction & 0x3F])(nextInstruction); \
		} else { \
			(this->*RTYPE_DECODER_TABLE[nextInstruction & 0x3F])(nextInstruction); \
		} \
	} else { \
		mCycles++; \
	}

#define RTYPE_D_REG mRegs[(instruction >> 11) & 31]
#define RTYPE_S_REG mRegs[(instruction >> 21) & 31]
#define RTYPE_T_REG mRegs[(instruction >> 16) & 31]


void MIPSR3000::Reset()
{
	// ToDo: implement
}


void MIPSR3000::Run(uint32_t maxCycles)
{
	while (mCycles < maxCycles) {
		uint32_t instruction = mMemory->ReadWord(mPC);
		mPC += 4;
		if (instruction & 0x0FC000000) {
			// I/J-type
			(this->*IJTYPE_DECODER_TABLE[instruction & 0x3F])(instruction);
		} else {
			// R-type
			(this->*RTYPE_DECODER_TABLE[instruction & 0x3F])(instruction);
		}
	}

}



void MIPSR3000::RTypeAnd(uint32_t instruction)
{
	RTYPE_D_REG = RTYPE_S_REG & RTYPE_T_REG;
	mCycles++;
}


void MIPSR3000::RTypeAddu(uint32_t instruction)
{
	RTYPE_D_REG = RTYPE_S_REG + RTYPE_T_REG;
	mCycles++;
}
