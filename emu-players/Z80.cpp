/*
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

#define NLOG_LEVEL_VERBOSE 0

#include <string>
#include <stdio.h>
#include "NativeLogger.h"
#include "Z80.h"


// LD R,N
#define MOVE_REG8_IMM8(R) \
	R = mMemory->ReadByte(mRegs.PC++); \
	mCycles += 7

// LD Q,A
#define MOVE_REG16_IMM16(Qhi, Qlo) \
	Qlo = mMemory->ReadByte(mRegs.PC++); \
	Qhi = mMemory->ReadByte(mRegs.PC++); \
	mCycles += 10

// LD R1,R2
#define MOVE_REG8_REG8(R1, R2) \
	R1 = R2; \
	mCycles += 4


void Z80::Reset()
{
	// TODO: implement
	mHalted = false;
}


void Z80::Run(uint32_t maxCycles)
{
	uint32_t temp32;
	uint16_t addr, temp16;
	uint8_t operand, temp8;
    int8_t relAddr;

	while ((mCycles < maxCycles) && !mHalted) {

		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);

		switch (opcode) {
		}
	}
}
