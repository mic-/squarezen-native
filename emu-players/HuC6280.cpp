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
#include "HuC6280.h"

#define UPDATE_NZ(val) mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_N); \
				       mRegs.F |= ((uint8_t)val == 0) ? HuC6280::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80)

// ====

#define COND_BRANCH(cc) \
	if ((cc)) { relAddr = mMemory->ReadByte(mRegs.PC++); \
	addr = mRegs.PC + relAddr; \
    mCycles += 2; \
    mRegs.PC = addr; } else { mRegs.PC++; } \
    mCycles += 2

// ====

#define ILLEGAL_OP() NLOGE("HuC6280", "Run(): Illegal opcode: %#x at PC=%#x", opcode, mRegs.PC); \
					 mCycles += 2; \
					 mRegs.PC++

// ====


void HuC6280::Reset()
{
	// ToDo: implement
}


void HuC6280::Run(uint32_t maxCycles)
{
	uint16_t addr, temp16;
	uint8_t operand, temp8;
    int8_t relAddr;

	while (mCycles < maxCycles) {

		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);

		switch (opcode) {
		case 0x02:	// SXY
			temp8 = mRegs.Y;
			mRegs.Y = mRegs.X;
			mRegs.X = temp8;
			mRegs.F &= ~FLAG_T;
			mCycles += 3;
			break;

		case 0x10:	// BPL rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_N) == 0);
			break;
		case 0x18:	// CLC
			mRegs.F &= ~(FLAG_C | FLAG_T);
			mCycles += 2;
			break;

		case 0x22:	// SAX
			temp8 = mRegs.A;
			mRegs.A = mRegs.X;
			mRegs.X = temp8;
			mRegs.F &= ~FLAG_T;
			mCycles += 3;
			break;

		case 0x30:	// BMI rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_N);
			break;
		case 0x38:	// SEC
			mRegs.F &= ~FLAG_T;
			mRegs.F |= FLAG_C;
			mCycles += 2;
			break;

		case 0x42:	// SAY
			temp8 = mRegs.A;
			mRegs.A = mRegs.Y;
			mRegs.Y = temp8;
			mRegs.F &= ~FLAG_T;
			mCycles += 3;
			break;

		case 0x50:	// BVC rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_V) == 0);
			break;
		case 0x54:	// CSL
			// ToDo: tell player that the CPU speed has changed
			if (mSpeed) {
				maxCycles >>= 1;
			}
			mSpeed = 0;
			mCycles += 3;
			break;
		case 0x58:	// CLI
			mRegs.F &= ~(FLAG_I | FLAG_T);
			mCycles += 2;
			break;

		case 0x62:	// CLA
			mRegs.A = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;

		case 0x70:	// BVS rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_V);
			break;
		case 0x78:	// SEI
			mRegs.F &= ~FLAG_T;
			mRegs.F |= FLAG_I;
			mCycles += 2;
			break;

		case 0x82:	// CLX
			mRegs.X = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;

		case 0x90:	// BCC rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_C) == 0);
			break;

		case 0xA8:	// TAY
			mRegs.Y = mRegs.A;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;
		case 0xAA:	// TAX
			mRegs.X = mRegs.A;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0xB0:	// BCS rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_C);
			break;
		case 0xB8:	// CLV
			mRegs.F &= ~(FLAG_V | FLAG_T);
			mCycles += 2;
			break;

		case 0xC2:	// CLY
			mRegs.Y = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;

		case 0xD0:	// BNE rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_Z) == 0);
			break;
		case 0xD4:	// CSH
			// ToDo: tell player that the CPU speed has changed
			if (!mSpeed) {
				maxCycles <<= 1;
			}
			mSpeed = 1;
			mCycles += 3;
			break;
		case 0xD8:	// CLD
			mRegs.F &= ~(FLAG_D | FLAG_T);
			mCycles += 2;
			break;

		case 0xF0:	// BEQ rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_Z);
			break;
		case 0xF4:	// SET
			mRegs.F |= FLAG_T;
			mCycles += 2;
			break;
		case 0xF8:	// SED
			mRegs.F &= ~FLAG_T;
			mRegs.F |= FLAG_D;
			mCycles += 2;
			break;

		default:
			ILLEGAL_OP();
			break;
		}
	}
}


void HuC6280PsgChannel::Reset()
{
	// ToDo: implement
}

void HuC6280PsgChannel::Step()
{
	// ToDo: implement
}


void HuC6280Psg::Reset()
{
	// ToDo: implement
}


void HuC6280Psg::Step()
{
	// ToDo: implement
	for (int i = 0; i < 6; i++) {
		mChannels[i].Step();
	}
}


void HuC6280Psg::Write(uint32_t addr, uint8_t data)
{
	// ToDo: implement
}
