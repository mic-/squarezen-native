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
		case 0x58:	// CLI
			mRegs.F &= ~(FLAG_I | FLAG_T);
			mCycles += 2;
			break;
		case 0x62:	// CLA
			mRegs.A = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
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
		case 0xB8:	// CLV
			mRegs.F &= ~(FLAG_V | FLAG_T);
			mCycles += 2;
			break;
		case 0xC2:	// CLY
			mRegs.Y = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;
		case 0xD8:	// CLD
			mRegs.F &= ~(FLAG_D | FLAG_T);
			mCycles += 2;
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
