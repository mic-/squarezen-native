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

#define UPDATE_NZC(val, val2, sub) mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_N | HuC6280::FLAG_C); \
				       mRegs.F |= ((uint8_t)val == (uint8_t)val2) ? HuC6280::FLAG_Z : 0; \
				       mRegs.F |= ((val - sub) & 0x80); \
				       mRegs.F |= (val >= val2) ? HuC6280::FLAG_C : 0
// ====

#define ADC(val) \
	if (mRegs.F & HuC6280::FLAG_T) { \
	  temp8 = mMemory->ReadByte(mRegs.X); \
 	  temp16 = (uint16_t)(val) + (uint16_t)temp8 + ((mRegs.F & HuC6280::FLAG_C) ? 1 : 0); \
	  UPDATE_NZC(temp16, 0x100, 0); \
	  mRegs.F &= ~(HuC6280::FLAG_V | HuC6280::FLAG_T); \
	  mRegs.F |= ((!((temp8 ^ val) & 0x80)) && ((temp8 ^ temp16) & 0x80)) ? HuC6280::FLAG_V : 0; \
	  mMemory->WriteByte(mRegs.X, (uint8_t)temp16); \
	} else { \
 	  temp16 = (uint16_t)(val) + (uint16_t)mRegs.A + ((mRegs.F & HuC6280::FLAG_C) ? 1 : 0); \
	  UPDATE_NZC(temp16, 0x100, 0); \
	  mRegs.F &= ~(HuC6280::FLAG_V | HuC6280::FLAG_T); \
	  mRegs.F |= ((!((mRegs.A ^ val) & 0x80)) && ((mRegs.A ^ temp16) & 0x80)) ? HuC6280::FLAG_V : 0; \
	  mRegs.A = (uint8_t)temp16; \
    }

#define AND(val) \
	if (mRegs.F & HuC6280::FLAG_T) { \
	    temp8 = mMemory->ReadByte(mRegs.X); \
		temp8 &= val; \
		UPDATE_NZ(temp8); \
  	    mMemory->WriteByte(mRegs.X, temp8); \
	} else { \
		mRegs.A &= val; \
		UPDATE_NZ(mRegs.A); \
	} \
	mRegs.F &= ~HuC6280::FLAG_T

#define EOR(val) \
	if (mRegs.F & HuC6280::FLAG_T) { \
	    temp8 = mMemory->ReadByte(mRegs.X); \
		temp8 ^= val; \
		UPDATE_NZ(temp8); \
  	    mMemory->WriteByte(mRegs.X, temp8); \
	} else { \
		mRegs.A ^= val; \
		UPDATE_NZ(mRegs.A); \
	} \
	mRegs.F &= ~HuC6280::FLAG_T

#define ORA(val) \
	if (mRegs.F & HuC6280::FLAG_T) { \
	    temp8 = mMemory->ReadByte(mRegs.X); \
		temp8 |= val; \
		UPDATE_NZ(temp8); \
  	    mMemory->WriteByte(mRegs.X, temp8); \
	} else { \
		mRegs.A |= val; \
		UPDATE_NZ(mRegs.A); \
	} \
	mRegs.F &= ~HuC6280::FLAG_T

#define BIT(val) \
	mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_V | HuC6280::FLAG_N | HuC6280::FLAG_T); \
	mRegs.F |= (((uint8_t)val & mRegs.A) == 0) ? HuC6280::FLAG_Z : 0; \
	mRegs.F |= (val & 0xC0)

// == Addressing ==

// abs
// Doesn't update PC
#define ABS_ADDR() (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8))

// abs,X
// Updates PC
#define ABSX_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    mRegs.PC += 2; \
						dest += mRegs.X

// abs,Y
// Updates PC
#define ABSY_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    mRegs.PC += 2; \
						dest += mRegs.Y

// zp
// Updates PC
#define ZP_ADDR() (mMemory->ReadByte(mRegs.PC++))

// zp,X
// Updates PC
#define ZPX_ADDR() ((ZP_ADDR() + mRegs.X) & 0xFF)

// zp,Y
// Updates PC
#define ZPY_ADDR() ((ZP_ADDR() + mRegs.Y) & 0xFF)

// (zp)
// Updates PC
#define IND_ADDR(dest)  dest = ZP_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte((dest & 0xFF00)+((dest+1)&0xFF)) << 8))

// (zp,X)
// Updates PC
#define INDX_ADDR(dest) dest = ZPX_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte((dest & 0xFF00)+((dest+1)&0xFF)) << 8))

// (zp),Y
// Updates PC
#define INDY_ADDR(dest) dest = ZP_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte((dest & 0xFF00)+((dest+1)&0xFF)) << 8)); \
						if ((dest & 0x100) != ((dest + mRegs.Y) & 0x100)) mCycles++; \
						dest += mRegs.Y


// ====

#define COND_BRANCH(cc) \
	if ((cc)) { relAddr = mMemory->ReadByte(mRegs.PC++); \
	addr = mRegs.PC + relAddr; \
    mCycles += 2; \
    mRegs.PC = addr; } else { mRegs.PC++; } \
    mCycles += 2

#define PUSHB(val) \
	mMemory->WriteByte(0x100 + (uint16_t)mRegs.S, val); \
	mRegs.S--

#define PUSHW(val) \
	mMemory->WriteByte(0x100 + (uint16_t)mRegs.S, ((uint16_t)val >> 8)); \
	mRegs.S--; \
	mMemory->WriteByte(0x100 + (uint16_t)mRegs.S, (uint8_t)val); \
	mRegs.S--

#define PULLB(dest) \
	mRegs.S++; \
	dest = mMemory->ReadByte(0x100 + (uint16_t)mRegs.S)

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
		case 0x01:	// ORA (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ORA(operand);
			mCycles += 7;
			break;
		case 0x02:	// SXY
			temp8 = mRegs.Y;
			mRegs.Y = mRegs.X;
			mRegs.X = temp8;
			mRegs.F &= ~FLAG_T;
			mCycles += 3;
			break;
		case 0x05:	// ORA zp
			operand = mMemory->ReadByte(ZP_ADDR());
			ORA(operand);
			mCycles += 4;
			break;
		case 0x09:	// ORA imm
			operand = mMemory->ReadByte(mRegs.PC++);
			ORA(operand);
			mCycles += 2;
			break;
		case 0x0D:	// ORA abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			ORA(operand);
			mCycles += 5;
			break;

		case 0x10:	// BPL rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_N) == 0);
			break;
		case 0x11:	// ORA (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ORA(operand);
			mCycles += 7;
			break;
		case 0x15:	// ORA zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			ORA(operand);
			mCycles += 4;
			break;
		case 0x18:	// CLC
			mRegs.F &= ~(FLAG_C | FLAG_T);
			mCycles += 2;
			break;
		case 0x19:	// ORA abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ORA(operand);
			mCycles += 5;
			break;
		case 0x1A:	// INC A
			mRegs.A++;
			UPDATE_NZ(mRegs.X);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		case 0x1D:	// ORA abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ORA(operand);
			mCycles += 5;
			break;

		case 0x20:	// JSR abs
			 addr = mMemory->ReadByte(mRegs.PC++);
			 addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			 PUSHW(mRegs.PC);
			 mRegs.PC = addr;
			 mCycles += 7;
			 break;
		case 0x21:	// AND (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			AND(operand);
			mCycles += 7;
			break;
		case 0x22:	// SAX
			temp8 = mRegs.A;
			mRegs.A = mRegs.X;
			mRegs.X = temp8;
			mRegs.F &= ~FLAG_T;
			mCycles += 3;
			break;
		case 0x24:	// BIT zp
			operand = mMemory->ReadByte(ZP_ADDR());
			BIT(operand);
			mCycles += 4;
			break;
		case 0x25:	// AND zp
			operand = mMemory->ReadByte(ZP_ADDR());
			AND(operand);
			mCycles += 4;
			break;
		case 0x29:	// AND imm
			operand = mMemory->ReadByte(mRegs.PC++);
			AND(operand);
			mCycles += 2;
			break;
		case 0x2C:	// BIT abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			BIT(operand);
			mCycles += 5;
			break;
		case 0x2D:	// AND abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			AND(operand);
			mCycles += 4;
			break;

		case 0x30:	// BMI rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_N);
			break;
		case 0x31:	// AND (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			AND(operand);
			mCycles += 7;
			break;
		case 0x35:	// AND zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			AND(operand);
			mCycles += 4;
			break;
		case 0x38:	// SEC
			mRegs.F &= ~FLAG_T;
			mRegs.F |= FLAG_C;
			mCycles += 2;
			break;
		case 0x39:	// AND abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			AND(operand);
			mCycles += 5;
			break;
		case 0x3D:	// AND abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			AND(operand);
			mCycles += 5;
			break;

		case 0x41:		// EOR (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 7;
			break;
		case 0x42:	// SAY
			temp8 = mRegs.A;
			mRegs.A = mRegs.Y;
			mRegs.Y = temp8;
			mRegs.F &= ~FLAG_T;
			mCycles += 3;
			break;
		case 0x44:	// BSR rel
			relAddr = mMemory->ReadByte(mRegs.PC++);
			addr = mRegs.PC + relAddr;
			PUSHW(mRegs.PC);
		    mRegs.PC = addr;
		    mCycles += 8;
			break;
		case 0x45:	// EOR zp
			operand = mMemory->ReadByte(ZP_ADDR());
			EOR(operand);
			mCycles += 4;
			break;
		case 0x49:	// EOR imm
			operand = mMemory->ReadByte(mRegs.PC++);
			EOR(operand);
			mCycles += 2;
			break;
		case 0x4D:	// EOR abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			EOR(operand);
			mCycles += 5;
			break;

		case 0x50:	// BVC rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_V) == 0);
			break;
		case 0x51:	// EOR (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 7;
			break;
		case 0x54:	// CSL
			// ToDo: tell player that the CPU speed has changed
			if (mSpeed) {
				maxCycles >>= 1;
			}
			mSpeed = 0;
			mCycles += 3;
			break;
		case 0x55:	// EOR zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			EOR(operand);
			mCycles += 4;
			break;
		case 0x58:	// CLI
			mRegs.F &= ~(FLAG_I | FLAG_T);
			mCycles += 2;
			break;
		case 0x59:	// EOR abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 5;
			break;
		case 0x5D:	// EOR abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 5;
			break;

		case 0x61:		// ADC (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 7;
			break;
		case 0x62:	// CLA
			mRegs.A = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;
		case 0x65:		// ADC zp
			operand = mMemory->ReadByte(ZP_ADDR());
			ADC(operand);
			mCycles += 4;
			break;
		case 0x69:		// ADC imm
			operand = mMemory->ReadByte(mRegs.PC++);
			ADC(operand);
			mCycles += 2;
			break;
		case 0x6D:		// ADC abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			ADC(operand);
			mCycles += 5;
			break;


		case 0x70:	// BVS rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_V);
			break;
		case 0x71:		// ADC (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 7;
			break;
		case 0x72:		// ADC (zp)
			IND_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 7;
			break;
		case 0x75:		// ADC zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			ADC(operand);
			mCycles += 4;
			break;
		case 0x78:	// SEI
			mRegs.F &= ~FLAG_T;
			mRegs.F |= FLAG_I;
			mCycles += 2;
			break;
		case 0x79:		// ADC abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 5;
			break;
		case 0x7D:		// ADC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 5;
			break;

		case 0x80:	// BRA rel
			COND_BRANCH(true);
			break;
		case 0x82:	// CLX
			mRegs.X = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;
		case 0x88:	// DEY
			mRegs.Y--;
			UPDATE_NZ(mRegs.Y);
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;
		case 0x89:	// BIT imm
			operand = mMemory->ReadByte(mRegs.PC++);
			BIT(operand);
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

		case 0xC0:	// CPY imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.Y, operand, operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		case 0xC2:	// CLY
			mRegs.Y = 0;
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;
		case 0xC4:	// CPY zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.Y, operand, operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0xC6:	// DEC zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xC8:	// INY
			mRegs.Y++;
			UPDATE_NZ(mRegs.Y);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		case 0xCA:	// DEX
			mRegs.X--;
			UPDATE_NZ(mRegs.X);
			mRegs.F &= ~FLAG_T;
			mCycles += 2;
			break;
		case 0xCC:	// CPY abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.Y, operand, operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0xCE:	// DEC abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
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
		case 0xD6:	// DEC zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xD8:	// CLD
			mRegs.F &= ~(FLAG_D | FLAG_T);
			mCycles += 2;
			break;
		case 0xDE:	// DEC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;

		case 0xE0:	// CPX imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.X, operand, operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		case 0xE1:	// SBC (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 7;
			break;
		case 0xE4:	// CPX zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.X, operand, operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0xE5:	// SBC zp
			operand = mMemory->ReadByte(ZP_ADDR()) ^ 0xFF;
			ADC(operand);
			mCycles += 4;
			break;
		case 0xE6:	// INC zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xE8:	// INX
			mRegs.X++;
			UPDATE_NZ(mRegs.X);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		case 0xE9:	// SBC imm
			operand = mMemory->ReadByte(mRegs.PC++) ^ 0xFF;
			ADC(operand);
			mCycles += 2;
			break;
		case 0xEC:	// CPX abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.X, operand, operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0xED:	// SBC abs
			operand = mMemory->ReadByte(ABS_ADDR()) ^ 0xFF;
			mRegs.PC += 2;
			ADC(operand);
			mCycles += 5;
			break;
		case 0xEE:	// INC abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;

		case 0xF0:	// BEQ rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_Z);
			break;
		case 0xF1:	// SBC (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 7;
			break;
		case 0xF2:	// SBC (zp)
			IND_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 7;
			break;
		case 0xF4:	// SET
			mRegs.F |= FLAG_T;
			mCycles += 2;
			break;
		case 0xF5:	// SBC zp,X
			operand = mMemory->ReadByte(ZPX_ADDR()) ^ 0xFF;
			ADC(operand);
			mCycles += 4;
			break;
		case 0xF6:	// INC zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xF8:	// SED
			mRegs.F &= ~FLAG_T;
			mRegs.F |= FLAG_D;
			mCycles += 2;
			break;
		case 0xF9:	// SBC abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 5;
			break;
		case 0xFD:	// SBC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 5;
			break;
		case 0xFE:	// INC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mRegs.F &= ~HuC6280::FLAG_T;
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
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
