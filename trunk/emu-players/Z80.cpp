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

static uint8_t gParityTable[256];


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

// LD R,J
#define MOVE_REG8_IXREG8(R1, J) \
	R1 = J; \
	mCycles += 8

#define CMD_GROUP_1OP(operation, base, R1) \
	operation(R1, mRegs.B); \
	break; \
	case base+1: \
	operation(R1, mRegs.C); \
	break; \
	case base+2: \
	operation(R1, mRegs.D); \
	break; \
	case base+3: \
	operation(R1, mRegs.E); \
	break; \
	case base+4: \
	operation(R1, mRegs.H); \
	break; \
	case base+5: \
	operation(R1, mRegs.L); \
	break; \
	case base+6: \
	operand = mMemory->ReadByte(((uint16_t)mRegs.H << 8) + mRegs.L); \
	operation(R1, operand); \
	mCycles += 3; \
	break; \
	case base+7: \
	operation(R1, mRegs.A); \
	break

#define CMD_GROUP_J_R(operation, base, J) \
	operation(J, mRegs.B); \
	break; \
	case base+1: \
	operation(J, mRegs.C); \
	break; \
	case base+2: \
	operation(J, mRegs.D); \
	break; \
	case base+3: \
	operation(J, mRegs.E); \
	break; \
	case base+7: \
	operation(J, mRegs.A); \
	break

#define CMD_GROUP_R_J(operation, base, J) \
	operation(mRegs.B, J); \
	break; \
	case base+1*8: \
	operation(mRegs.C, J); \
	break; \
	case base+2*8: \
	operation(mRegs.D, J); \
	break; \
	case base+3*8: \
	operation(mRegs.E, J); \
	break; \
	case base+4*8: \
	operation(mRegs.H, J); \
	break; \
	case base+5*8: \
	operation(mRegs.L, J); \
	break; \
	case base+7*8: \
	operation(mRegs.A,J); \
	break

// ====

#define AND8(R1, R2) \
	R1 &= R2;\
	mRegs.F = (R1 & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y)) | Z80::FLAG_H; \
	mRegs.F |= gParityTable[R1]; \
	mRegs.F |= (R1 == 0) ? Z80::FLAG_Z : 0; \
	mCycles += 4

#define OR8(R1, R2) \
	R1 |= R2;\
	mRegs.F = R1 & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R1]; \
	mRegs.F |= (R1 == 0) ? Z80::FLAG_Z : 0; \
	mCycles += 4

#define XOR8(R1, R2) \
	R1 ^= R2;\
	mRegs.F = R1 & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R1]; \
	mRegs.F |= (R1 == 0) ? Z80::FLAG_Z : 0; \
	mCycles += 4

// ====

#define ADD8(dest8, val) \
	temp16 = (uint16_t)(val) + (uint16_t)dest8; \
	mRegs.F = temp16 & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= ((temp16 >= 0x100) ? Z80::FLAG_C : 0); \
	mRegs.F |= (((uint8_t)temp16 == 0) ? Z80::FLAG_Z : 0); \
	mRegs.F |= ((!((dest8 ^ val) & 0x80)) && ((dest8 ^ temp16) & 0x80)) ? Z80::FLAG_P : 0; \
	dest8 = (uint8_t)temp16

#define ADC8(dest8, val) \
	temp16 = (uint16_t)(val) + (uint16_t)dest8 + ((mRegs.F & Z80::FLAG_C) ? 1 : 0); \
	mRegs.F = temp16 & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= ((temp16 >= 0x100) ? Z80::FLAG_C : 0); \
	mRegs.F |= (((uint8_t)temp16 == 0) ? Z80::FLAG_Z : 0); \
	mRegs.F |= ((!((dest8 ^ val) & 0x80)) && ((dest8 ^ temp16) & 0x80)) ? Z80::FLAG_P : 0; \
	dest8 = (uint8_t)temp16

// ====

#define ILLEGAL_OP() NLOGE("Z80", "Run(): Illegal opcode: %#x at PC=%#x", opcode, mRegs.PC); \
					 mCycles += 2; \
					 mRegs.PC++

#define ILLEGAL_OP2() NLOGE("Z80", "Run(): Illegal opcode: %#x%#x at PC=%#x", opcode, opcode2, mRegs.PC); \
					 mCycles += 2; \
					 mRegs.PC++

// ====

static uint8_t parity(uint8_t val)
{
	uint8_t result = val ^
			         (val >> 1) ^
			         (val >> 2) ^
			         (val >> 3) ^
			         (val >> 4) ^
			         (val >> 5) ^
			         (val >> 6) ^
			         (val >> 7);
	// FLAG_P if val had an even number of bits set; 0 otherwise
	return (result & 1) ? 0 : Z80::FLAG_P;
}


Z80::Z80()
{
	for (int i = 0; i < 256; i++) {
		gParityTable[i] = parity(i);
	}
	mHalted = false;
}


void Z80::Reset()
{
	// TODO: implement
	mHalted = false;
}


void Z80::Run(uint32_t maxCycles)
{
	uint32_t temp32;
	uint16_t addr, temp16;
	uint8_t opcode2, operand, temp8;
    int8_t relAddr;

	while ((mCycles < maxCycles) && !mHalted) {

		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);

		switch (opcode) {
		case 0x00:	// NOP
			mCycles += 4;
			break;
		case 0x01:	// LD BC,aaaa
			MOVE_REG16_IMM16(mRegs.B, mRegs.C);
			break;
		case 0x11:	// LD DE,aaaa
			MOVE_REG16_IMM16(mRegs.D, mRegs.E);
			break;
		case 0x21:	// LD HL,aaaa
			MOVE_REG16_IMM16(mRegs.H, mRegs.L);
			break;
		case 0x31:	// LD SP,aaaa
			MOVE_REG16_IMM16(operand, temp8);
			mRegs.SP = (((uint16_t)operand) << 8) | temp8;
			break;

		case 0x40:	// LD B,R2
			CMD_GROUP_1OP(MOVE_REG8_REG8, 0x40, mRegs.B);
			break;
		case 0x48:	// LD C,R2
			CMD_GROUP_1OP(MOVE_REG8_REG8, 0x48, mRegs.C);
			break;
		case 0x50:	// LD D,R2
			CMD_GROUP_1OP(MOVE_REG8_REG8, 0x50, mRegs.D);
			break;
		case 0x58:	// LD E,R2
			CMD_GROUP_1OP(MOVE_REG8_REG8, 0x58, mRegs.E);
			break;
		case 0x60:	// LD H,R2
			CMD_GROUP_1OP(MOVE_REG8_REG8, 0x60, mRegs.H);
			break;
		case 0x68:	// LD L,R2
			CMD_GROUP_1OP(MOVE_REG8_REG8, 0x68, mRegs.L);
			break;
		case 0x78:	// LD A,R2
			CMD_GROUP_1OP(MOVE_REG8_REG8, 0x78, mRegs.A);
			break;

		case 0x80:	// ADD A,R2
			CMD_GROUP_1OP(ADD8, 0x80, mRegs.A);
			break;
		case 0x88:	// ADC A,R2
			CMD_GROUP_1OP(ADC8, 0x88, mRegs.A);
			break;

		case 0xA0:	// AND A,R2
			CMD_GROUP_1OP(AND8, 0xA0, mRegs.A);
			break;
		case 0xA8:	// XOR A,R2
			CMD_GROUP_1OP(XOR8, 0xA8, mRegs.A);
			break;
		case 0xB0:	// OR A,R2
			CMD_GROUP_1OP(OR8, 0xB0, mRegs.A);
			break;

		case 0xDD:	// IX prefix
			opcode2 = mMemory->ReadByte(mRegs.PC++);
			switch (opcode2) {
			case 0x44:	// LD R,IXH
				CMD_GROUP_R_J(MOVE_REG8_IXREG8, 0x44, mRegs.ixh);
				break;
			case 0x45:	// LD R,IXL
				CMD_GROUP_R_J(MOVE_REG8_IXREG8, 0x45, mRegs.ixl);
				break;
			case 0x60:	// LD IXH,R
				CMD_GROUP_J_R(MOVE_REG8_IXREG8, 0x60, mRegs.ixh);
				break;
			case 0x68:	// LD IXL,R
				CMD_GROUP_J_R(MOVE_REG8_IXREG8, 0x68, mRegs.ixl);
				break;
			default:
				ILLEGAL_OP2();
				break;
			}
			break;

		case 0xFD:	// IY prefix
			opcode2 = mMemory->ReadByte(mRegs.PC++);
			switch (opcode2) {
			case 0x44:	// LD R,IYH
				CMD_GROUP_R_J(MOVE_REG8_IXREG8, 0x44, mRegs.iyh);
				break;
			case 0x45:	// LD R,IYL
				CMD_GROUP_R_J(MOVE_REG8_IXREG8, 0x45, mRegs.iyl);
				break;
			case 0x60:	// LD IYH,R
				CMD_GROUP_J_R(MOVE_REG8_IXREG8, 0x60, mRegs.iyh);
				break;
			case 0x68:	// LD IYL,R
				CMD_GROUP_J_R(MOVE_REG8_IXREG8, 0x68, mRegs.iyl);
				break;
			default:
				ILLEGAL_OP2();
				break;
			}
			break;

		default:
			ILLEGAL_OP();
			break;
		}
	}
}
