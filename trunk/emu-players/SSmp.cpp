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
#include "SSmp.h"


#define UPDATE_NZ(val) mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N); \
				       mRegs.PSW |= ((uint8_t)val == 0) ? SSmp::FLAG_Z : 0; \
				       mRegs.PSW |= (val & 0x80)

// zp
// Updates PC
#define ZP_ADDR() (mMemory->ReadByte(mRegs.PC++))

// zp,X
// Updates PC
#define ZPX_ADDR() ((ZP_ADDR() + mRegs.X) & 0xFF)


#define PUSHB(val) mMemory->WriteByte(0x100 + (uint16_t)mRegs.SP, val); \
	               mRegs.SP--

#define PUSHW(val) mMemory->WriteByte(0x100 + (uint16_t)mRegs.SP, ((uint16_t)val >> 8)); \
	               mRegs.SP--; \
	               mMemory->WriteByte(0x100 + (uint16_t)mRegs.SP, (uint8_t)val); \
	               mRegs.SP--

#define PULLB(dest) mRegs.SP++; \
	                dest = mMemory->ReadByte(0x100 + (uint16_t)mRegs.SP)

// ====


void SSmp::Reset()
{
	// TODO: implement
	mHalted = false;
}

void SSmp::Run(uint32_t maxCycles)
{
	uint16_t addr, temp16;
	uint8_t operand, temp8;
    int8_t relAddr;

	while ((mCycles < maxCycles) && !mHalted) {

		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);

		switch (opcode) {

		// == MOV imm/reg==
		case 0xE8:		// MOV A,#nn
			mRegs.A = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0xCD:		// MOV X,#nn
			mRegs.X = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		case 0x8D:		// MOV Y,#nn
			mRegs.Y = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0x7D:		// MOV A,X
			mRegs.A = mRegs.X;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0x5D:		// MOV X,A
			mRegs.X = mRegs.A;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		case 0xDD:		// MOV A,Y
			mRegs.A = mRegs.Y;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0xFD:		// MOV Y,A
			mRegs.Y = mRegs.A;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0x9D:		// MOV X,SP
			mRegs.X = mRegs.SP;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		case 0xBD:		// MOV SP,X
			mRegs.SP = mRegs.X;
			mCycles += 2;
			break;

		// == MOV mem ==
		case 0xE4:		// MOV A,aa
			mRegs.A = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0xF4:		// MOV A,aa,X
			mRegs.A = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		// == Push/Pop ==
		case 0x2D:		// PUSH A
			PUSHB(mRegs.A);
			mCycles += 4;
			break;
		case 0x4D:		// PUSH X
			PUSHB(mRegs.X);
			mCycles += 4;
			break;
		case 0x6D:		// PUSH Y
			PUSHB(mRegs.Y);
			mCycles += 4;
			break;
		case 0x0D:		// PUSH pSW
			PUSHB(mRegs.PSW);
			mCycles += 4;
			break;

		case 0xAE:		// POP A
			PULLB(mRegs.A);
			mCycles += 4;
			break;
		case 0xCE:		// POP X
			PULLB(mRegs.X);
			mCycles += 4;
			break;
		case 0xEE:		// POP Y
			PULLB(mRegs.Y);
			mCycles += 4;
			break;
		case 0x8E:		// POP PSW
			PULLB(mRegs.PSW);
			mCycles += 4;
			break;

		// == Wait/delay/control ==
		case 0x00:
			mCycles += 2;
			break;

		case 0xEF:		// SLEEP
			mHalted = true;
			break;
		case 0xFF:		// STOP
			mHalted = true;
			break;

		case 0x20:		// CLRP
			mRegs.PSW &= ~SSmp::FLAG_P;
			mCycles += 2;
			break;
		case 0x40:		// SETP
			mRegs.PSW |= SSmp::FLAG_P;
			mCycles += 2;
			break;

		case 0xA0:		// EI
			mRegs.PSW |= SSmp::FLAG_I;
			mCycles += 3;
			break;
		case 0xC0:		// DI
			mRegs.PSW &= ~SSmp::FLAG_I;
			mCycles += 3;
			break;

		// ====
		default:
			break;
		}
	}
}



