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

// CMD R/(HL)
#define CMD_GROUP_1OP(operation, base) \
	operation(mRegs.B); \
	break; \
	case base+1: \
	operation(mRegs.C); \
	break; \
	case base+2: \
	operation(mRegs.D); \
	break; \
	case base+3: \
	operation(mRegs.E); \
	break; \
	case base+4: \
	operation(mRegs.H); \
	break; \
	case base+5: \
	operation(mRegs.L); \
	break; \
	case base+6: \
	addr = ((uint16_t)mRegs.H << 8) + mRegs.L; \
	operand = mMemory->ReadByte(addr); \
	operation(operand); \
	mMemory->WriteByte(addr, operand); \
	mCycles += 7; \
	break; \
	case base+7: \
	operation(mRegs.A); \
	break

// CMD A,R/(HL)
#define CMD_GROUP_2OP(operation, base, R1) \
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

// CMD J,R
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

// CMD R,J
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
	mRegs.F |= ((temp16 & 0xF) < (dest8 & 0xF)) ? Z80::FLAG_H : 0; \
	mRegs.F |= ((!((dest8 ^ val) & 0x80)) && ((dest8 ^ temp16) & 0x80)) ? Z80::FLAG_P : 0; \
	dest8 = (uint8_t)temp16; \
	mCycles += 4

#define ADC8(dest8, val) \
	temp16 = (uint16_t)(val) + (uint16_t)dest8 + ((mRegs.F & Z80::FLAG_C) ? 1 : 0); \
	mRegs.F = temp16 & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= ((temp16 >= 0x100) ? Z80::FLAG_C : 0); \
	mRegs.F |= (((uint8_t)temp16 == 0) ? Z80::FLAG_Z : 0); \
	mRegs.F |= ((!((dest8 ^ val) & 0x80)) && ((dest8 ^ temp16) & 0x80)) ? Z80::FLAG_P : 0; \
	dest8 = (uint8_t)temp16; \
	mCycles += 4

#define SUB8(dest8, val) \
	operand = -(val); \
	ADD8(dest8, operand); \
	mRegs.F |= Z80::FLAG_N

#define CP8(dest, src) \
	temp8 = dest; \
	SUB8(temp8, src); \
	mRegs.F &= ~(Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= src & (Z80::FLAG_X | Z80::FLAG_Y)

#define NEG8(dest8) \
	temp8 = 0; \
	SUB8(temp8, dest8); \
	mCycles += 4

// ====

#define RL8(R) \
	temp8 = (R & 0x80) ? Z80::FLAG_C : 0;\
    R = (R << 1) | (mRegs.F & Z80::FLAG_C); \
	mRegs.F = R & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R]; \
	mRegs.F |= temp8; \
    mRegs.F |= (R == 0) ? Z80::FLAG_Z : 0; \
    mCycles += 8

#define RR8(R) \
	temp8 = (R & 0x01) ? Z80::FLAG_C : 0; \
    R = (R >> 1) | ((mRegs.F & Z80::FLAG_C) << 7); \
	mRegs.F = R & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R]; \
	mRegs.F |= temp8; \
    mRegs.F |= (R == 0) ? Z80::FLAG_Z : 0; \
    mCycles += 8

#define SRA8(R) \
	mRegs.F = (R & 0x01) ? Z80::FLAG_C : 0; \
    R = (R >> 1) | (R & 0x80); \
	mRegs.F |= R & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R]; \
    mRegs.F |= (R == 0) ? Z80::FLAG_Z : 0; \
    mCycles += 8

#define SRL8(R) \
	mRegs.F = (R & 0x01) ? Z80::FLAG_C : 0; \
    R >>= 1; \
	mRegs.F |= R & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R]; \
    mRegs.F |= (R == 0) ? Z80::FLAG_Z : 0; \
    mCycles += 8

#define SLA8(R) \
	mRegs.F = (R & 0x01) ? Z80::FLAG_C : 0; \
    R <<= 1; \
	mRegs.F |= R & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R]; \
    mRegs.F |= (R == 0) ? Z80::FLAG_Z : 0; \
    mCycles += 8

#define SLL8(R) \
	mRegs.F = (R & 0x01) ? Z80::FLAG_C : 0; \
    R = (R << 1) + 1; \
	mRegs.F |= R & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R]; \
    mRegs.F |= (R == 0) ? Z80::FLAG_Z : 0; \
    mCycles += 8

// ====

#define IN8_C(R) \
	R = mMemory->ReadPort(mRegs.C); \
    mRegs.F &= Z80::FLAG_C; \
	mRegs.F |= R & (Z80::FLAG_S | Z80::FLAG_X | Z80::FLAG_Y); \
	mRegs.F |= gParityTable[R]; \
    mRegs.F |= (R == 0) ? Z80::FLAG_Z : 0; \
	mCycles += 12

#define OUT8_C(R) \
	mMemory->WritePort(mRegs.C, R); \
	mCycles += 12

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
		case 0x08:	// EX AF,AF'
			temp8 = mRegs.A;
			mRegs.A = mRegs.A2;
			mRegs.A2 = temp8;
			temp8 = mRegs.F;
			mRegs.F = mRegs.F2;
			mRegs.F2 = temp8;
			mCycles += 4;
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
			CMD_GROUP_2OP(MOVE_REG8_REG8, 0x40, mRegs.B);
			break;
		case 0x48:	// LD C,R2
			CMD_GROUP_2OP(MOVE_REG8_REG8, 0x48, mRegs.C);
			break;
		case 0x50:	// LD D,R2
			CMD_GROUP_2OP(MOVE_REG8_REG8, 0x50, mRegs.D);
			break;
		case 0x58:	// LD E,R2
			CMD_GROUP_2OP(MOVE_REG8_REG8, 0x58, mRegs.E);
			break;
		case 0x60:	// LD H,R2
			CMD_GROUP_2OP(MOVE_REG8_REG8, 0x60, mRegs.H);
			break;
		case 0x68:	// LD L,R2
			CMD_GROUP_2OP(MOVE_REG8_REG8, 0x68, mRegs.L);
			break;
		case 0x76:	// HALT
			mHalted = true;
			mCycles += 4;
			break;
		case 0x78:	// LD A,R2
			CMD_GROUP_2OP(MOVE_REG8_REG8, 0x78, mRegs.A);
			break;

		case 0x80:	// ADD A,R2
			CMD_GROUP_2OP(ADD8, 0x80, mRegs.A);
			break;
		case 0x88:	// ADC A,R2
			CMD_GROUP_2OP(ADC8, 0x88, mRegs.A);
			break;
		case 0x90:	// SUB A,R2
			CMD_GROUP_2OP(SUB8, 0x90, mRegs.A);
			break;

		case 0xA0:	// AND A,R2
			CMD_GROUP_2OP(AND8, 0xA0, mRegs.A);
			break;
		case 0xA8:	// XOR A,R2
			CMD_GROUP_2OP(XOR8, 0xA8, mRegs.A);
			break;
		case 0xB0:	// OR A,R2
			CMD_GROUP_2OP(OR8, 0xB0, mRegs.A);
			break;
		case 0xB8:	// CP A,R2
			CMD_GROUP_2OP(CP8, 0xB8, mRegs.A);
			break;

		case 0xCB:	// bit instruction prefix
			opcode2 = mMemory->ReadByte(mRegs.PC++);
			switch (opcode2) {
			case 0x10:	// RL R
				CMD_GROUP_1OP(RL8, 0x10);
				break;
			case 0x18:	// RR R
				CMD_GROUP_1OP(RR8, 0x18);
				break;
			case 0x20:	// SLA R
				CMD_GROUP_1OP(SLA8, 0x20);
				break;
			case 0x28:	// SRA R
				CMD_GROUP_1OP(SRA8, 0x28);
				break;
			case 0x30:	// SLL R
				CMD_GROUP_1OP(SLL8, 0x30);
				break;
			case 0x38:	// SRL R
				CMD_GROUP_1OP(SRL8, 0x38);
				break;
			default:
				ILLEGAL_OP2();
				break;
			}
			break;

		case 0xD3:	// OUT (N),A
			addr = mMemory->ReadByte(mRegs.PC++);
			mMemory->WritePort(addr, mRegs.A);
			mCycles += 11;
			break;

		case 0xDB:	// IN A,(N)
			addr = mMemory->ReadByte(mRegs.PC++);
			mRegs.A = mMemory->ReadPort(addr);
			mCycles += 11;
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
			case 0x84:	// ADD A,IXH
				ADD8(mRegs.A, mRegs.ixh);
				mCycles += 4;
				break;
			case 0x85:	// ADD A,IXL
				ADD8(mRegs.A, mRegs.ixl);
				mCycles += 4;
				break;
			case 0x8C:	// ADC A,IXH
				ADC8(mRegs.A, mRegs.ixh);
				mCycles += 4;
				break;
			case 0x8D:	// ADC A,IXL
				ADC8(mRegs.A, mRegs.ixl);
				mCycles += 4;
				break;
			case 0x94:	// SUB A,IXH
				SUB8(mRegs.A, mRegs.ixh);
				mCycles += 4;
				break;
			case 0x95:	// SUB A,IXL
				SUB8(mRegs.A, mRegs.ixl);
				mCycles += 4;
				break;
			case 0xA4:	// AND A,IXH
				AND8(mRegs.A, mRegs.ixh);
				mCycles += 4;
				break;
			case 0xA5:	// AND A,IXL
				AND8(mRegs.A, mRegs.ixl);
				mCycles += 4;
				break;
			case 0xAC:	// XOR A,IXH
				XOR8(mRegs.A, mRegs.ixh);
				mCycles += 4;
				break;
			case 0xAD:	// XOR A,IXL
				XOR8(mRegs.A, mRegs.ixl);
				mCycles += 4;
				break;
			case 0xB4:	// OR A,IXH
				OR8(mRegs.A, mRegs.ixh);
				mCycles += 4;
				break;
			case 0xB5:	// OR A,IXL
				OR8(mRegs.A, mRegs.ixl);
				mCycles += 4;
				break;
			case 0xBC:	// CP A,IXH
				CP8(mRegs.A, mRegs.ixh);
				mCycles += 4;
				break;
			case 0xBD:	// CP A,IXL
				CP8(mRegs.A, mRegs.ixl);
				mCycles += 4;
				break;
			default:
				ILLEGAL_OP2();
				break;
			}
			break;

		case 0xEB:	// EX DE,HL
			temp8 = mRegs.D;
			mRegs.D = mRegs.H;
			mRegs.H = temp8;
			temp8 = mRegs.E;
			mRegs.E = mRegs.L;
			mRegs.L = temp8;
			mCycles += 4;
			break;

		case 0xED:	// ED prefix
			opcode2 = mMemory->ReadByte(mRegs.PC++);
			switch (opcode2) {
			case 0x40:	// IN B,(C)
				IN8_C(mRegs.B);
				break;
			case 0x48:	// IN C,(C)
				IN8_C(mRegs.C);
				break;
			case 0x50:	// IN D,(C)
				IN8_C(mRegs.D);
				break;
			case 0x58:	// IN E,(C)
				IN8_C(mRegs.E);
				break;
			case 0x60:	// IN H,(C)
				IN8_C(mRegs.H);
				break;
			case 0x68:	// IN L,(C)
				IN8_C(mRegs.L);
				break;
			case 0x78:	// IN A,(C)
				IN8_C(mRegs.A);
				break;

			case 0x41:	// OUT (C),B
				OUT8_C(mRegs.B);
				break;
			case 0x49:	// OUT (C),C
				OUT8_C(mRegs.C);
				break;
			case 0x51:	// OUT (C),D
				OUT8_C(mRegs.D);
				break;
			case 0x59:	// OUT (C),E
				OUT8_C(mRegs.E);
				break;
			case 0x61:	// OUT (C),H
				OUT8_C(mRegs.H);
				break;
			case 0x69:	// OUT (C),L
				OUT8_C(mRegs.L);
				break;
			case 0x79:	// OUT (C),A
				OUT8_C(mRegs.A);
				break;

			case 0x44:
			case 0x4C:
			case 0x54:
			case 0x5C:
			case 0x64:
			case 0x6C:
			case 0x74:
			case 0x7C:
				NEG8(mRegs.A);
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
			case 0x84:	// ADD A,IYH
				ADD8(mRegs.A, mRegs.iyh);
				mCycles += 4;
				break;
			case 0x85:	// ADD A,IYL
				ADD8(mRegs.A, mRegs.iyl);
				mCycles += 4;
				break;
			case 0x8C:	// ADC A,IYH
				ADC8(mRegs.A, mRegs.iyh);
				mCycles += 4;
				break;
			case 0x8D:	// ADC A,IYL
				ADC8(mRegs.A, mRegs.iyl);
				mCycles += 4;
				break;
			case 0x94:	// SUB A,IYH
				SUB8(mRegs.A, mRegs.iyh);
				mCycles += 4;
				break;
			case 0x95:	// SUB A,IYL
				SUB8(mRegs.A, mRegs.iyl);
				mCycles += 4;
				break;
			case 0xA4:	// AND A,IYH
				AND8(mRegs.A, mRegs.iyh);
				mCycles += 4;
				break;
			case 0xA5:	// AND A,IYL
				AND8(mRegs.A, mRegs.iyl);
				mCycles += 4;
				break;
			case 0xAC:	// XOR A,IYH
				XOR8(mRegs.A, mRegs.iyh);
				mCycles += 4;
				break;
			case 0xAD:	// XOR A,IYL
				XOR8(mRegs.A, mRegs.iyl);
				mCycles += 4;
				break;
			case 0xB4:	// OR A,IYH
				OR8(mRegs.A, mRegs.iyh);
				mCycles += 4;
				break;
			case 0xB5:	// OR A,IYL
				OR8(mRegs.A, mRegs.iyl);
				mCycles += 4;
				break;
			case 0xBC:	// CP A,IYH
				CP8(mRegs.A, mRegs.iyh);
				mCycles += 4;
				break;
			case 0xBD:	// CP A,IYL
				CP8(mRegs.A, mRegs.iyl);
				mCycles += 4;
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
