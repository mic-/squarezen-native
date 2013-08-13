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
#define ZP_ADDR() (mMemory->ReadByte(mRegs.PC++) + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))

// zp+X
// Updates PC
#define ZPX_ADDR() (((mMemory->ReadByte(mRegs.PC++) + mRegs.X) & 0xFF) + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))

// zp+Y
// Updates PC
#define ZPY_ADDR() (((mMemory->ReadByte(mRegs.PC++) + mRegs.Y) & 0xFF) + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))


// !abs
// Doesn't update PC
#define ABS_ADDR() (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8))

// !abs+X
// Updates PC
#define ABSX_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    mRegs.PC += 2; \
						dest += mRegs.X

// !abs+Y
// Updates PC
#define ABSY_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    mRegs.PC += 2; \
						dest += mRegs.Y

// ====

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

		// == MOV reg,mem ==
		case 0xE4:		// MOV A,aa
			mRegs.A = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0xF4:		// MOV A,aa+X
			mRegs.A = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0xE5:		// MOV A,!aaaa
			mRegs.A = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0xF5:		// MOV A,!aaaa+X
			ABSX_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;

		case 0xF6:		// MOV A,!aaaa+Y
			ABSY_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;

		case 0xF8:		// MOV X,aa
			mRegs.X = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.X);
			mCycles += 3;
			break;

		case 0xF9:		// MOV X,aa+Y
			mRegs.X = mMemory->ReadByte(ZPY_ADDR());
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;

		case 0xE6:		// MOV A,(X)
			mRegs.A = mMemory->ReadByte(mRegs.X);
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0xBF:		// MOV A,(X)+
			mRegs.A = mMemory->ReadByte(mRegs.X++);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0xE9:		// MOV X,!aaaa
			mRegs.A = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;

		case 0xEB:		// MOV Y,aa
			mRegs.Y = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.Y);
			mCycles += 3;
			break;

		case 0xFB:		// MOV Y,aa+X
			mRegs.Y = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;

		case 0xEC:		// MOV Y,!aaaa
			mRegs.Y = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;

		// == MOV mem,reg/imm ==
		case 0x8F:		// MOV aa,#nn
			operand = mMemory->ReadByte(mRegs.PC++);
			mMemory->WriteByte(ZP_ADDR(), operand);
			mCycles += 5;
			break;

		case 0xFA:		// MOV aa,bb
			operand = mMemory->ReadByte(ZP_ADDR());
			mMemory->WriteByte(ZP_ADDR(), operand);
			mCycles += 5;
			break;

		case 0xC4:		// MOV aa,A
			mMemory->WriteByte(ZP_ADDR(), mRegs.A);
			mCycles += 4;
			break;

		case 0xD8:		// MOV aa,X
			mMemory->WriteByte(ZP_ADDR(), mRegs.X);
			mCycles += 4;
			break;

		case 0xCB:		// MOV aa,Y
			mMemory->WriteByte(ZP_ADDR(), mRegs.Y);
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



