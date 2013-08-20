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

#define UPDATE_NZC(val, val2, sub) mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N | SSmp::FLAG_C); \
				       mRegs.PSW |= ((uint8_t)val == (uint8_t)val2) ? SSmp::FLAG_Z : 0; \
				       mRegs.PSW |= ((val - sub) & 0x80); \
				       mRegs.PSW |= (val >= val2) ? SSmp::FLAG_C : 0

// zp
// Updates PC
#define ZP_ADDR() (mMemory->ReadByte(mRegs.PC++) + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))

// zp+X
// Updates PC
#define ZPX_ADDR() (((mMemory->ReadByte(mRegs.PC++) + mRegs.X) & 0xFF) + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))

// zp+Y
// Updates PC
#define ZPY_ADDR() (((mMemory->ReadByte(mRegs.PC++) + mRegs.Y) & 0xFF) + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))

// (X)
#define X_ADDR() (mRegs.X + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))

// (Y)
#define Y_ADDR() (mRegs.Y + ((mRegs.PSW & SSmp::FLAG_P) ? 0x100 : 0))

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

// [aa+X]
// Updates PC
#define INDX_ADDR(dest) dest = ZPX_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte(dest+1) << 8))

// [aa]+Y
// Updates PC
#define INDY_ADDR(dest) dest = ZP_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte(dest+1) << 8)); \
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

// == Bitwise logic ==

#define BITWISE_aa_bb(op) \
	operand = mMemory->ReadByte(ZP_ADDR()); \
	addr = ZP_ADDR(); \
	temp8 = mMemory->ReadByte(addr) op operand; \
	mMemory->WriteByte(addr, temp8); \
	UPDATE_NZ(temp8); \
	mCycles += 6

#define BITWISE_aa_imm(op) \
	operand = mMemory->ReadByte(mRegs.PC++); \
	addr = ZP_ADDR(); \
	temp8 = mMemory->ReadByte(addr) & operand; \
	mMemory->WriteByte(addr, temp8); \
	UPDATE_NZ(temp8); \
	mCycles += 5

#define BITWISE_atX_atY(op) \
	operand = mMemory->ReadByte(Y_ADDR()); \
	addr = X_ADDR(); \
	temp8 = mMemory->ReadByte(addr) & operand; \
	mMemory->WriteByte(addr, temp8); \
	UPDATE_NZ(temp8); \
	mCycles += 5

#define BITWISE_A_aaaa(op) \
	mRegs.A = mRegs.A op mMemory->ReadByte(ABS_ADDR()); \
	mRegs.PC += 2; \
	UPDATE_NZ(mRegs.A); \
	mCycles += 4

#define BITWISE_A_aaaaX(op) \
	ABSX_ADDR(addr); \
	mRegs.A = mRegs.A op mMemory->ReadByte(addr); \
	UPDATE_NZ(mRegs.A); \
	mCycles += 5

#define BITWISE_A_aaaaY(op) \
	ABSY_ADDR(addr); \
	mRegs.A = mRegs.A op mMemory->ReadByte(addr); \
	UPDATE_NZ(mRegs.A); \
	mCycles += 5

#define BITWISE_A_indX(op) \
	INDX_ADDR(addr); \
	mRegs.A = mRegs.A op mMemory->ReadByte(addr); \
	UPDATE_NZ(mRegs.A); \
	mCycles += 6

#define BITWISE_A_indY(op) \
	INDY_ADDR(addr); \
	mRegs.A = mRegs.A op mMemory->ReadByte(addr); \
	UPDATE_NZ(mRegs.A); \
	mCycles += 6

// == Arithmetic ==

#define ADC(dest8, val) \
	temp16 = (uint16_t)(val) + (uint16_t)dest8 + ((mRegs.PSW & SSmp::FLAG_C) ? 1 : 0); \
	UPDATE_NZC(temp16, 0x100, 0); \
	mRegs.PSW &= ~SSmp::FLAG_V; \
	mRegs.PSW |= ((!((dest8 ^ val) & 0x80)) && ((dest8 ^ temp16) & 0x80)) ? SSmp::FLAG_V : 0; \
	dest8 = (uint8_t)temp16

#define ASL(val) \
	mRegs.PSW &= ~SSmp::FLAG_C; \
	mRegs.PSW |= (val & 0x80) ? SSmp::FLAG_C : 0; \
	val <<= 1; \
	UPDATE_NZ(val)

#define LSR(val) \
	mRegs.PSW &= ~SSmp::FLAG_C; \
	mRegs.PSW |= (val & 0x01) ? SSmp::FLAG_C : 0; \
	val >>= 1; \
	UPDATE_NZ(val)

#define ROL(val) \
	temp8 = (val << 1) | ((mRegs.PSW & SSmp::FLAG_C) ? 1 : 0); \
	mRegs.PSW &= ~SSmp::FLAG_C; \
	mRegs.PSW |= (val & 0x80) ? SSmp::FLAG_C : 0; \
	val = temp8; \
	UPDATE_NZ(val)

#define ROR(val) \
	temp8 = (val >> 1) | ((mRegs.PSW & SSmp::FLAG_C) ? 0x80 : 0); \
	mRegs.PSW &= ~SSmp::FLAG_C; \
	mRegs.PSW |= (val & 0x01) ? SSmp::FLAG_C : 0; \
	val = temp8; \
	UPDATE_NZ(val)

// ====

#define CLR1_aa_b(b) \
	addr = ZP_ADDR(); \
	mMemory->WriteByte(addr, mMemory->ReadByte(addr) & ~(1 << b)); \
	mCycles += 4

#define SET1_aa_b(b) \
	addr = ZP_ADDR(); \
	mMemory->WriteByte(addr, mMemory->ReadByte(addr) | (1 << b)); \
	mCycles += 4

// ====

#define COND_BRANCH(cc) if ((cc)) { relAddr = mMemory->ReadByte(mRegs.PC++); \
								    addr = mRegs.PC + relAddr; \
                                    mCycles += 2; \
                                    mRegs.PC = addr; } else { mRegs.PC++; } \
                                  mCycles += 2

// ====

#define ILLEGAL_OP() NLOGE("SSmp", "Run(): Illegal opcode: %#x at PC=%#x", opcode, mRegs.PC); \
					 mCycles += 2; \
					 mRegs.PC++

// ====


void SSmp::Reset()
{
	// TODO: implement
	mHalted = false;
}


void SSmp::Run(uint32_t maxCycles)
{
	uint32_t temp32;
	uint16_t addr, temp16;
	uint8_t operand, temp8;
    int8_t relAddr;

	while ((mCycles < maxCycles) && !mHalted) {

		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);

		NLOGD("SSmp", "%04x: %#x, A=%#x, X=%#x, Y=%#x, SP=%#x, PSW=%#x", mRegs.PC-1, opcode, mRegs.A, mRegs.X, mRegs.Y, mRegs.SP, mRegs.PSW);

		switch (opcode) {

		// == Arithmetic ==
		case 0x88:		// ADC A,#nn
			operand = mMemory->ReadByte(mRegs.PC++);
			ADC(mRegs.A, operand);
			mCycles += 2;
			break;
		case 0x86:		// ADC A,(X)
			operand = mMemory->ReadByte(X_ADDR());
			ADC(mRegs.A, operand);
			mCycles += 3;
			break;
		case 0x84:		// ADC A,aa
			operand = mMemory->ReadByte(ZP_ADDR());
			ADC(mRegs.A, operand);
			mCycles += 3;
			break;
		case 0x94:		// ADC A,aa+X
			operand = mMemory->ReadByte(ZPX_ADDR());
			ADC(mRegs.A, operand);
			mCycles += 4;
			break;
		case 0x85:		// ADC A,!aaaa
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			ADC(mRegs.A, operand);
			mCycles += 4;
			break;
		case 0x95:		// ADC A,!aaaa+X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(mRegs.A, operand);
			mCycles += 5;
			break;
		case 0x96:		// ADC A,!aaaa+Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(mRegs.A, operand);
			mCycles += 5;
			break;
		case 0x87:		// ADC A,[aa+X]
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(mRegs.A, operand);
			mCycles += 6;
			break;
		case 0x97:		// ADC A,[aa]+Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(mRegs.A, operand);
			mCycles += 6;
			break;

		case 0xA8:		// SBC A,#nn
			operand = mMemory->ReadByte(mRegs.PC++) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 2;
			break;
		case 0xA6:		// SBC A,(X)
			operand = mMemory->ReadByte(X_ADDR()) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 3;
			break;
		case 0xA4:		// SBC A,aa
			operand = mMemory->ReadByte(ZP_ADDR()) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 3;
			break;
		case 0xB4:		// SBC A,aa+X
			operand = mMemory->ReadByte(ZPX_ADDR()) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 4;
			break;
		case 0xA5:		// SBC A,!aaaa
			operand = mMemory->ReadByte(ABS_ADDR()) ^ 0xFF;
			mRegs.PC += 2;
			ADC(mRegs.A, operand);
			mCycles += 4;
			break;
		case 0xB5:		// SBC A,!aaaa+X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 5;
			break;
		case 0xB6:		// SBC A,!aaaa+Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 5;
			break;
		case 0xA7:		// SBC A,[aa+X]
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 6;
			break;
		case 0xB7:		// SBC A,[aa]+Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(mRegs.A, operand);
			mCycles += 6;
			break;

		// == Rotate/shift ==
		case 0x1C:		// ASL A
			ASL(mRegs.A);
			mCycles += 2;
			break;
		case 0x0B:		// ASL aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 4;
			break;
		case 0x1B:		// ASL aa+X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0x0C:		// ASL !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x5C:		// LSR A
			LSR(mRegs.A);
			mCycles += 2;
			break;
		case 0x4B:		// LSR aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 4;
			break;
		case 0x5B:		// LSR aa+X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0x4C:		// LSR !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x3C:		// ROL A
			ROL(mRegs.A);
			mCycles += 2;
			break;
		case 0x2B:		// ROL aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 4;
			break;
		case 0x3B:		// ROL aa+X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0x2C:		// ROL !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x7C:		// ROR A
			ROR(mRegs.A);
			mCycles += 2;
			break;
		case 0x6B:		// ROR aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 4;
			break;
		case 0x7B:		// ROR aa+X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0x6C:		// ROR !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x9C:		// DEC A
			mRegs.A--;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0x8B:		// DEC aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 4;
			break;
		case 0x9B:		// DEC aa+X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0x8C:		// DEC !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0x1D:		// DEC X
			mRegs.X--;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		case 0xDC:		// DEC Y
			mRegs.Y--;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0xBC:		// INC A
			mRegs.A++;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0xAB:		// INC aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 4;
			break;
		case 0xBB:		// INC aa+X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0xAC:		// INC !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;
		case 0x3D:		// INC X
			mRegs.X++;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		case 0xFC:		// INC Y
			mRegs.Y++;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		// == 16-bit ALU operations ==
		case 0x7A:		// ADDW YA,aa
			addr = ZP_ADDR();
			temp16 = mMemory->ReadByte(addr);
			temp16 |= (uint16_t)(mMemory->ReadByte(addr+1)) << 8;
			addr = (((uint16_t)mRegs.Y) << 8) | mRegs.A;
			temp32 = temp16 + addr;
			mRegs.A = temp32 & 0xFF;
			mRegs.Y = (temp32 >> 8) & 0xFF;
			mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N | SSmp::FLAG_C | SSmp::FLAG_V);
			mRegs.PSW |= ((uint16_t)temp32 == 0) ? SSmp::FLAG_Z : 0;
			mRegs.PSW |= (mRegs.Y & 0x80);
			mRegs.PSW |= (temp32 >= 0x10000) ? SSmp::FLAG_C : 0;
			mRegs.PSW |= ((!((addr ^ temp16) & 0x8000)) && ((addr ^ temp32) & 0x8000)) ? SSmp::FLAG_V : 0;
			mCycles += 5;
			break;
		case 0x9A:		// SUBW YA,aa
			addr = ZP_ADDR();
			temp16 = mMemory->ReadByte(addr);
			temp16 |= (uint16_t)(mMemory->ReadByte(addr+1)) << 8;
			addr = (((uint16_t)mRegs.Y) << 8) | mRegs.A;
			addr = -addr;
			temp32 = temp16 + addr;
			mRegs.A = temp32 & 0xFF;
			mRegs.Y = (temp32 >> 8) & 0xFF;
			mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N | SSmp::FLAG_C | SSmp::FLAG_V);
			mRegs.PSW |= ((uint16_t)temp32 == 0) ? SSmp::FLAG_Z : 0;
			mRegs.PSW |= (mRegs.Y & 0x80);
			mRegs.PSW |= (temp32 >= 0x10000) ? SSmp::FLAG_C : 0;
			mRegs.PSW |= ((!((addr ^ temp16) & 0x8000)) && ((addr ^ temp32) & 0x8000)) ? SSmp::FLAG_V : 0;
			mCycles += 5;
			break;

		case 0x1A:		// DECW aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			temp8 = mMemory->ReadByte(addr+1);
			if (operand == 0xFF) temp8--;
			mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N);
			mRegs.PSW |= (operand == 0 && temp8 == 0) ? SSmp::FLAG_Z : 0;
			mRegs.PSW |= (temp8 & 0x80);
			mMemory->WriteByte(addr, operand);
			mMemory->WriteByte(addr+1, temp8);
			mCycles += 6;
			break;
		case 0x3A:		// INCW aa
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) + 1;
			temp8 = mMemory->ReadByte(addr+1);
			if (operand == 0x00) temp8++;
			mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N);
			mRegs.PSW |= (operand == 0 && temp8 == 0) ? SSmp::FLAG_Z : 0;
			mRegs.PSW |= (temp8 & 0x80);
			mMemory->WriteByte(addr, operand);
			mMemory->WriteByte(addr+1, temp8);
			mCycles += 6;
			break;

		case 0xCF:		// MUL YA
			temp16 = mRegs.Y * mRegs.A;
			mRegs.A = temp16 & 0xFF;
			mRegs.Y = (temp16 >> 8) & 0xFF;
			UPDATE_NZ(mRegs.Y);
			mCycles += 9;
			break;

		// == 1-bit ALU operations ==
		case 0x12:		// CLR1 aa.0
			CLR1_aa_b(0);
			break;
		case 0x32:		// CLR1 aa.1
			CLR1_aa_b(1);
			break;
		case 0x52:		// CLR1 aa.2
			CLR1_aa_b(2);
			break;
		case 0x72:		// CLR1 aa.3
			CLR1_aa_b(3);
			break;
		case 0x92:		// CLR1 aa.4
			CLR1_aa_b(4);
			break;
		case 0xB2:		// CLR1 aa.5
			CLR1_aa_b(5);
			break;
		case 0xD2:		// CLR1 aa.6
			CLR1_aa_b(6);
			break;
		case 0xF2:		// CLR1 aa.7
			CLR1_aa_b(7);
			break;

		case 0x02:		// SET1 aa.0
			SET1_aa_b(0);
			break;
		case 0x22:		// SET1 aa.1
			SET1_aa_b(1);
			break;
		case 0x42:		// SET1 aa.2
			SET1_aa_b(2);
			break;
		case 0x62:		// SET1 aa.3
			SET1_aa_b(3);
			break;
		case 0x82:		// SET1 aa.4
			SET1_aa_b(4);
			break;
		case 0xA2:		// SET1 aa.5
			SET1_aa_b(5);
			break;
		case 0xC2:		// SET1 aa.6
			SET1_aa_b(6);
			break;
		case 0xE2:		// SET1 aa.7
			SET1_aa_b(7);
			break;

		case 0xAA:		// MOV1 C,aaa.b
			operand = mMemory->ReadByte(mRegs.PC++);
			addr = mMemory->ReadByte(mRegs.PC++);
			temp8 = (addr >> 5) & 0x07;
			addr = ((addr & 0x1F) << 8) | operand;
			mRegs.PSW &= ~SSmp::FLAG_C;
			mRegs.PSW |= ((mMemory->ReadByte(addr) & (1 << temp8)) ? SSmp::FLAG_C : 0);
			mCycles += 4;
			break;
		case 0xCA:		// MOV1 aaa.b,C
			operand = mMemory->ReadByte(mRegs.PC++);
			addr = mMemory->ReadByte(mRegs.PC++);
			temp8 = (addr >> 5) & 0x07;
			addr = ((addr & 0x1F) << 8) | operand;
			operand = mMemory->ReadByte(addr) & ~SSmp::FLAG_C;
			mMemory->WriteByte(addr, operand & ((mRegs.PSW & SSmp::FLAG_C) << temp8));
			mCycles += 5;
			break;

		case 0x60:		// CLRC
			mRegs.PSW &= ~SSmp::FLAG_C;
			mCycles += 2;
			break;
		case 0x80:		// SETC
			mRegs.PSW |= SSmp::FLAG_C;
			mCycles += 2;
			break;
		case 0xED:		// NOTC
			mRegs.PSW ^= SSmp::FLAG_C;
			mCycles += 2;
			break;

		case 0xE0:		// CLRV
			mRegs.PSW &= ~(SSmp::FLAG_V | SSmp::FLAG_H);
			mCycles += 2;
			break;

		// == Special ALU operations ==
		case 0x9F:		// XCN
			mRegs.A = (mRegs.A >> 4) | (mRegs.A << 4);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x4E:		// TCLR1 !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N);
	        mRegs.PSW |= ((uint8_t)mRegs.A == (uint8_t)operand) ? SSmp::FLAG_Z : 0;
	        mRegs.PSW |= ((mRegs.A - operand) & 0x80);
			mMemory->WriteByte(addr, operand & ~mRegs.A);
			mCycles += 6;
			break;
		case 0x0E:		// TSET1 !aaaa
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N);
	        mRegs.PSW |= ((uint8_t)mRegs.A == (uint8_t)operand) ? SSmp::FLAG_Z : 0;
	        mRegs.PSW |= ((mRegs.A - operand) & 0x80);
			mMemory->WriteByte(addr, operand | mRegs.A);
			mCycles += 6;
			break;

		// == Bitwise logic ==
		case 0x28:		// AND A,#nn
			mRegs.A &= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0x26:		// AND A,(X)
			mRegs.A &= mMemory->ReadByte(X_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;
		case 0x24:		// AND A,aa
			mRegs.A &= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;
		case 0x34:		// AND A,aa+X
			mRegs.A &= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;
		case 0x25:		// AND A,!aaaa
			BITWISE_A_aaaa(&);
			break;
		case 0x35:		// AND A,!aaaa+X
			BITWISE_A_aaaaX(&);
			break;
		case 0x36:		// AND A,!aaaa+Y
			BITWISE_A_aaaaY(&);
			break;
		case 0x27:		// AND A,[aa+X]
			BITWISE_A_indX(&);
			break;
		case 0x37:		// AND A,[aa]+Y
			BITWISE_A_indY(&);
			break;
		case 0x29:		// AND aa,bb
			BITWISE_aa_bb(&);
			break;
		case 0x38:		// AND aa,#nn
			BITWISE_aa_imm(&);
			break;
		case 0x39:		// AND (X),(Y)
			BITWISE_atX_atY(&);
			break;

		case 0x48:		// EOR A,#nn
			mRegs.A ^= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0x46:		// EOR A,(X)
			mRegs.A ^= mMemory->ReadByte(X_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;
		case 0x44:		// EOR A,aa
			mRegs.A ^= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;
		case 0x54:		// EOR A,aa+X
			mRegs.A ^= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;
		case 0x45:		// EOR A,!aaaa
			BITWISE_A_aaaa(^);
			break;
		case 0x55:		// EOR A,!aaaa+X
			BITWISE_A_aaaaX(^);
			break;
		case 0x56:		// EOR A,!aaaa+Y
			BITWISE_A_aaaaY(^);
			break;
		case 0x47:		// EOR A,[aa+X]
			BITWISE_A_indX(^);
			break;
		case 0x57:		// EOR A,[aa]+Y
			BITWISE_A_indY(^);
			break;
		case 0x49:		// EOR aa,bb
			BITWISE_aa_bb(^);
			break;
		case 0x58:		// EOR aa,#nn
			BITWISE_aa_imm(^);
			break;
		case 0x59:		// EOR (X),(Y)
			BITWISE_atX_atY(^);
			break;

		case 0x08:		// OR A,#nn
			mRegs.A |= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0x06:		// OR A,(X)
			mRegs.A |= mMemory->ReadByte(X_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;
		case 0x04:		// OR A,aa
			mRegs.A |= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;
		case 0x14:		// OR A,aa+X
			mRegs.A |= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;
		case 0x05:		// OR A,!aaaa
			BITWISE_A_aaaa(|);
			break;
		case 0x15:		// OR A,!aaaa+X
			BITWISE_A_aaaaX(|);
			break;
		case 0x16:		// OR A,!aaaa+Y
			BITWISE_A_aaaaY(|);
			break;
		case 0x07:		// OR A,[aa+X]
			BITWISE_A_indX(|);
			break;
		case 0x17:		// OR A,[aa]+Y
			BITWISE_A_indY(|);
			break;
		case 0x09:		// OR aa,bb
			BITWISE_aa_bb(|);
			break;
		case 0x18:		// OR aa,#nn
			BITWISE_aa_imm(|);
			break;
		case 0x19:		// OR (X),(Y)
			BITWISE_atX_atY(|);
			break;

		// == CMP ==
		case 0x68:		// CMP A,#nn
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 2;
			break;
		case 0x66:		// CMP A,(X)
			operand = mMemory->ReadByte(X_ADDR());
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 3;
			break;
		case 0x64:		// CMP A,aa
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 3;
			break;
		case 0x74:		// CMP aa+X
			operand = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 4;
			break;
		case 0x65:		// CMP A,!aaaa
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 4;
			break;
		case 0x75:		// CMP A,!aaaa+X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 5;
			break;
		case 0x76:		// CMP A,!aaaa+Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 5;
			break;
		case 0x67:		// CMP A,[aa+X]
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 6;
			break;
		case 0x77:		// CMP A,[aa]+Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 6;
			break;
		case 0x79:		// CMP (X),(Y)
			operand = mMemory->ReadByte(Y_ADDR());
			temp8 = mMemory->ReadByte(X_ADDR());
			UPDATE_NZC(temp8, operand, operand);
			mCycles += 5;
			break;
		case 0xC8:		// CMP X,#nn
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.X, operand, operand);
			mCycles += 2;
			break;
		case 0x3E:		// CMP X,aa
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.X, operand, operand);
			mCycles += 3;
			break;
		case 0xAD:		// CMP Y,#nn
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.Y, operand, operand);
			mCycles += 2;
			break;
		case 0x7E:		// CMP Y,aa
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.Y, operand, operand);
			mCycles += 3;
			break;

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
		case 0xE6:		// MOV A,(X)
			mRegs.A = mMemory->ReadByte(X_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;
		case 0xBF:		// MOV A,(X)+
			mRegs.A = mMemory->ReadByte(X_ADDR());
			mRegs.X++;
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;
		case 0xE7:		// MOV A,[aa+X]
			INDX_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 6;
			break;
		case 0xF7:		// MOV A,[aa]+Y
			INDY_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 6;
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
		case 0xBA:		// MOV YA,aa
			addr = ZP_ADDR();
			mRegs.A = mMemory->ReadByte(addr);
			mRegs.Y = mMemory->ReadByte(addr+1);
			mRegs.PSW &= ~(SSmp::FLAG_Z | SSmp::FLAG_N);
			mRegs.PSW |= (mRegs.A == 0 && mRegs.Y == 0) ? SSmp::FLAG_Z : 0;
			mRegs.PSW |= (mRegs.Y & 0x80);
			mCycles += 5;
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
		case 0xD4:		// MOV aa+X,A
			mMemory->WriteByte(ZPX_ADDR(), mRegs.A);
			mCycles += 5;
			break;
		case 0xC5:		// MOV !aaaa,A
			mMemory->WriteByte(ABS_ADDR(), mRegs.A);
			mRegs.PC += 2;
			mCycles += 5;
			break;
		case 0xD5:		// MOV !aaaa+X,A
			ABSX_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 6;
			break;
		case 0xD6:		// MOV !aaaa+Y,A
			ABSY_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 6;
			break;
		case 0xAF:		// MOV (X),A
			mMemory->WriteByte(X_ADDR(), mRegs.A);
			mCycles += 4;
			break;
		case 0xC6:		// MOV (X)+,A
			mMemory->WriteByte(X_ADDR(), mRegs.A);
			mRegs.X++;
			mCycles += 4;
			break;
		case 0xC7:		// MOV [aa+X],A
			INDX_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 7;
			break;
		case 0xD7:		// MOV [aa]+Y,A
			INDY_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 7;
			break;
		case 0xD8:		// MOV aa,X
			mMemory->WriteByte(ZP_ADDR(), mRegs.X);
			mCycles += 4;
			break;
		case 0xD9:		// MOV aa+Y,X
			mMemory->WriteByte(ZPY_ADDR(), mRegs.X);
			mCycles += 5;
			break;
		case 0xC9:		// MOV !aaaa,X
			mMemory->WriteByte(ABS_ADDR(), mRegs.X);
			mRegs.PC += 2;
			mCycles += 5;
			break;
		case 0xCB:		// MOV aa,Y
			mMemory->WriteByte(ZP_ADDR(), mRegs.Y);
			mCycles += 4;
			break;
		case 0xDB:		// MOV aa+X,Y
			mMemory->WriteByte(ZPX_ADDR(), mRegs.Y);
			mCycles += 5;
			break;
		case 0xCC:		// MOV !aaaa,Y
			mMemory->WriteByte(ABS_ADDR(), mRegs.Y);
			mRegs.PC += 2;
			mCycles += 5;
			break;
		case 0xDA:		// MOVW aa,YA
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, mRegs.A);
			mMemory->WriteByte(addr+1, mRegs.Y);
			mCycles += 5;
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

		// Conditional branches
		case 0x10:		// BPL rr
			COND_BRANCH((mRegs.PSW & SSmp::FLAG_N) == 0);
			break;
		case 0x30:		// BMI rr
			COND_BRANCH(mRegs.PSW & SSmp::FLAG_N);
			break;
		case 0x50:		// BVC rr
			COND_BRANCH((mRegs.PSW & SSmp::FLAG_V) == 0);
			break;
		case 0x70:		// BVS rr
			COND_BRANCH(mRegs.PSW & SSmp::FLAG_V);
			break;
		case 0x90:		// BCC rr
			COND_BRANCH((mRegs.PSW & SSmp::FLAG_C) == 0);
			break;
		case 0xB0:		// BCS rr
			COND_BRANCH(mRegs.PSW & SSmp::FLAG_C);
			break;
		case 0xD0:		// BNE rr
			COND_BRANCH((mRegs.PSW & SSmp::FLAG_Z) == 0);
			break;
		case 0xF0:		// BEQ rr
			COND_BRANCH(mRegs.PSW & SSmp::FLAG_Z);
			break;

		case 0x03:		// BBS aa.0,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x01);
			break;
		case 0x23:		// BBS aa.1,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x02);
			break;
		case 0x43:		// BBS aa.2,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x04);
			break;
		case 0x63:		// BBS aa.3,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x08);
			break;
		case 0x83:		// BBS aa.4,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x10);
			break;
		case 0xA3:		// BBS aa.5,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x20);
			break;
		case 0xC3:		// BBS aa.6,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x40);
			break;
		case 0xE3:		// BBS aa.7,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(operand & 0x80);
			break;

		case 0x13:		// BBC aa.0,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x01) == 0);
			break;
		case 0x33:		// BBC aa.1,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x02) == 0);
			break;
		case 0x53:		// BBC aa.2,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x04) == 0);
			break;
		case 0x73:		// BBC aa.3,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x08) == 0);
			break;
		case 0x93:		// BBC aa.4,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x10) == 0);
			break;
		case 0xB3:		// BBC aa.5,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x20) == 0);
			break;
		case 0xD3:		// BBC aa.6,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x40) == 0);
			break;
		case 0xF3:		// BBC aa.7,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH((operand & 0x80) == 0);
			break;

		case 0x2E:		// CBNE aa,rr
			operand = mMemory->ReadByte(ZP_ADDR());
			mCycles += 3;
			COND_BRANCH(mRegs.A != operand);
			break;
		case 0xDE:		// CBNE aa+X,rr
			operand = mMemory->ReadByte(ZPX_ADDR());
			mCycles += 4;
			COND_BRANCH(mRegs.A != operand);
			break;

		case 0xFE:		// DBNZ Y,rr
			mRegs.Y--;
			mCycles += 2;
			COND_BRANCH(mRegs.Y);
			break;
		case 0x6E:		// DBNZ aa,rr
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			operand--;
			mMemory->WriteByte(addr, operand);
			mCycles += 3;
			COND_BRANCH(operand);
			break;

		// Unconditional branches
		case 0x2F:		// BRA rr
			COND_BRANCH(true);
			break;

		case 0x5F:		// JMP !aaaa
			temp16 = mRegs.PC - 1;
			addr = mMemory->ReadByte(mRegs.PC++);
			addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			mRegs.PC = addr;
			/*if (addr == temp16) {
				// Exit early for infinite loops
				return;
			}*/
			mCycles += 3;
			break;

		case 0x3F:		// CALL !aaaa
			 addr = mMemory->ReadByte(mRegs.PC++);
			 addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			 PUSHW(mRegs.PC);
			 mRegs.PC = addr;
			 mCycles += 8;
			 break;

		case 0x4F:		// PCALL nn
			 addr = mMemory->ReadByte(mRegs.PC++) | 0xFF00;
			 PUSHW(mRegs.PC);
			 mRegs.PC = addr;
			 mCycles += 6;
			 break;

		case 0x6F:		// RET
			PULLB(mRegs.PC);
			PULLB(addr);
			mRegs.PC += (addr << 8) + 1;
			mCycles += 5;
			break;

		// ====
		default:
			ILLEGAL_OP();
			break;
		}
	}
}



