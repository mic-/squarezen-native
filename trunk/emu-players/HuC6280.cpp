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
#define NLOG_TAG "HuC6280"

#include <string>
#include <stdio.h>
#include "NativeLogger.h"
#include "HuC6280.h"
#include "HesMapper.h"
#include "HesPlayer.h"


#define UPDATE_NZ(val) mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_N | HuC6280::FLAG_T); \
				       mRegs.F |= ((uint8_t)val == 0) ? HuC6280::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80)

#define UPDATE_NZC(val, val2, sub) mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_N | HuC6280::FLAG_C | HuC6280::FLAG_T); \
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

// == Bitwise operations ==

#define AND(val) \
	if (mRegs.F & HuC6280::FLAG_T) { \
	    temp8 = mMemory->ReadByte(mRegs.X); \
		temp8 &= val; \
		UPDATE_NZ(temp8); \
  	    mMemory->WriteByte(mRegs.X, temp8); \
	} else { \
		mRegs.A &= val; \
		UPDATE_NZ(mRegs.A); \
	}

#define EOR(val) \
	if (mRegs.F & HuC6280::FLAG_T) { \
	    temp8 = mMemory->ReadByte(mRegs.X); \
		temp8 ^= val; \
		UPDATE_NZ(temp8); \
  	    mMemory->WriteByte(mRegs.X, temp8); \
	} else { \
		mRegs.A ^= val; \
		UPDATE_NZ(mRegs.A); \
	}

#define ORA(val) \
	if (mRegs.F & HuC6280::FLAG_T) { \
	    temp8 = mMemory->ReadByte(mRegs.X); \
		temp8 |= val; \
		UPDATE_NZ(temp8); \
  	    mMemory->WriteByte(mRegs.X, temp8); \
	} else { \
		mRegs.A |= val; \
		UPDATE_NZ(mRegs.A); \
	}

#define TST(val, mask) \
	mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_V | HuC6280::FLAG_N | HuC6280::FLAG_T); \
	mRegs.F |= (((uint8_t)val & mask) == 0) ? HuC6280::FLAG_Z : 0; \
	mRegs.F |= (val & 0xC0)

#define BIT(val) \
	TST(val, mRegs.A)

#define RMB(bit) \
	addr = ZP_ADDR(); \
	operand = mMemory->ReadByte(addr); \
	mRegs.F &= ~HuC6280::FLAG_T; \
	operand &= ~(1 << bit); \
	mMemory->WriteByte(addr, operand); \
	mCycles += 7;

#define SMB(bit) \
	addr = ZP_ADDR(); \
	operand = mMemory->ReadByte(addr); \
	mRegs.F &= ~HuC6280::FLAG_T; \
	operand |= (1 << bit); \
	mMemory->WriteByte(addr, operand); \
	mCycles += 7;

#define ASL(val) \
	mRegs.F &= ~HuC6280::FLAG_C; \
	mRegs.F |= (val & 0x80) ? HuC6280::FLAG_C : 0; \
	val <<= 1; \
	UPDATE_NZ(val)

#define LSR(val) \
	mRegs.F &= ~HuC6280::FLAG_C; \
	mRegs.F |= (val & 0x01) ? HuC6280::FLAG_C : 0; \
	val >>= 1; \
	UPDATE_NZ(val)

#define ROL(val) \
	temp8 = (val << 1) | ((mRegs.F & HuC6280::FLAG_C) ? 1 : 0); \
	mRegs.F &= ~HuC6280::FLAG_C; \
	mRegs.F |= (val & 0x80) ? HuC6280::FLAG_C : 0; \
	val = temp8; \
	UPDATE_NZ(val)

#define ROR(val) \
	temp8 = (val >> 1) | ((mRegs.F & HuC6280::FLAG_C) ? 0x80 : 0); \
	mRegs.F &= ~HuC6280::FLAG_C; \
	mRegs.F |= (val & 0x01) ? HuC6280::FLAG_C : 0; \
	val = temp8; \
	UPDATE_NZ(val)

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
    mRegs.F &= ~HuC6280::FLAG_T; \
    mCycles += 2

#define BRANCH_ON_BIT(bit, val) \
	operand = mMemory->ReadByte(mRegs.PC++); \
	if ((operand & (1 << bit)) == (val << bit)) { relAddr = mMemory->ReadByte(mRegs.PC++); \
	addr = mRegs.PC + relAddr; \
    mCycles += 2; \
    mRegs.PC = addr; \
	} else { \
	mRegs.PC++; \
	} \
    mRegs.F &= ~HuC6280::FLAG_T; \
    mCycles += 6

#define PUSHB(val) \
	mMemory->WriteByte(0x2100 + (uint16_t)mRegs.S, val); \
	mRegs.S--

#define PUSHW(val) \
	mMemory->WriteByte(0x2100 + (uint16_t)mRegs.S, ((uint16_t)val >> 8)); \
	mRegs.S--; \
	mMemory->WriteByte(0x2100 + (uint16_t)mRegs.S, (uint8_t)val); \
	mRegs.S--

#define PULLB(dest) \
	mRegs.S++; \
	dest = mMemory->ReadByte(0x2100 + (uint16_t)mRegs.S)

#define BLOCK_TRANSFER(destInc, srcInc, destAlt, srcAlt) \
	temp8 = 0; \
	addr = mMemory->ReadByte(mRegs.PC++); \
	addr += (uint16_t)(mMemory->ReadByte(mRegs.PC++)) << 8; \
	addr2 = mMemory->ReadByte(mRegs.PC++);	\
	addr2 += (uint16_t)(mMemory->ReadByte(mRegs.PC++)) << 8; \
	temp16 = mMemory->ReadByte(mRegs.PC++);	\
	temp16 += (uint16_t)(mMemory->ReadByte(mRegs.PC++)) << 8; \
	PUSHB(mRegs.Y); \
	PUSHB(mRegs.A); \
	PUSHB(mRegs.X); \
	do { \
		mMemory->WriteByte(addr + (temp8 & destAlt), mMemory->ReadByte(addr2 + (temp8 & srcAlt))); \
		addr += destInc; \
		addr2 += srcInc; \
		temp16--; \
		temp8 ^= 1; \
		mCycles += 6; \
	} while (temp16); \
	PULLB(mRegs.X); \
	PULLB(mRegs.A); \
	PULLB(mRegs.Y); \
	mRegs.F &= ~HuC6280::FLAG_T; \
	mCycles += 17

// ====

#define ILLEGAL_OP() NLOGE("HuC6280", "Run(): Illegal opcode: %#x at PC=%#x", opcode, mRegs.PC); \
					 mCycles += 2; \
					 mRegs.PC++

// ====

const HuC6280::Instruction gDisassemblyTable[256] =
{
	{0x00, "BRK",     HuC6280::NO_OPERANDS},
	{0x01, "ORA",     HuC6280::OPERAND_INDX},
	{0x02, "SXY",     HuC6280::NO_OPERANDS},
	{0x03, "ST0",     HuC6280::OPERAND_IMM},
	{0x04, "TSB",     HuC6280::OPERAND_ZP},
	{0x05, "ORA",     HuC6280::OPERAND_ZP},
	{0x06, "ASL",     HuC6280::OPERAND_ZP},
	{0x07, "RMB0",    HuC6280::OPERAND_ZP},
	{0x08, "PHP",     HuC6280::NO_OPERANDS},
	{0x09, "ORA",     HuC6280::OPERAND_IMM},
	{0x0A, "ASL",     HuC6280::OPERAND_ACCUM},
	{0x0B, "Illegal", HuC6280::NO_OPERANDS},
	{0x0C, "TSB",     HuC6280::OPERAND_ABS},
	{0x0D, "ORA",     HuC6280::OPERAND_ABS},
	{0x0E, "ASL",     HuC6280::OPERAND_ABS},
	{0x0F, "BBR0",    HuC6280::OPERAND_ZP_REL},
	{0x10, "BPL",     HuC6280::OPERAND_REL},
	{0x11, "ORA",     HuC6280::OPERAND_INDY},
	{0x12, "ORA",     HuC6280::OPERAND_IND},
	{0x13, "ST1",     HuC6280::OPERAND_IMM},
	{0x14, "TRB",     HuC6280::OPERAND_ZP},
	{0x15, "ORA",     HuC6280::OPERAND_ZPX},
	{0x16, "ASL",     HuC6280::OPERAND_ZPX},
	{0x17, "RMB1",    HuC6280::OPERAND_ZP},
	{0x18, "CLC",     HuC6280::NO_OPERANDS},
	{0x19, "ORA",     HuC6280::OPERAND_ABSY},
	{0x1A, "INC",     HuC6280::OPERAND_ACCUM},
	{0x1B, "Illegal", HuC6280::NO_OPERANDS},
	{0x1C, "TRB",     HuC6280::OPERAND_ABS},
	{0x1D, "ORA",     HuC6280::OPERAND_ABSX},
	{0x1E, "ASL",     HuC6280::OPERAND_ABSX},
	{0x1F, "BBR1",    HuC6280::OPERAND_ZP_REL},
	{0x20, "JSR",     HuC6280::OPERAND_ABS},
	{0x21, "AND",     HuC6280::OPERAND_INDX},
	{0x22, "SAX",     HuC6280::NO_OPERANDS},
	{0x23, "ST2",     HuC6280::OPERAND_IMM},
	{0x24, "BIT",     HuC6280::OPERAND_ZP},
	{0x25, "AND",     HuC6280::OPERAND_ZP},
	{0x26, "ROL",     HuC6280::OPERAND_ZP},
	{0x27, "RMB2",    HuC6280::OPERAND_ZP},
	{0x28, "PLP",     HuC6280::NO_OPERANDS},
	{0x29, "AND",     HuC6280::OPERAND_IMM},
	{0x2A, "ROL",     HuC6280::OPERAND_ACCUM},
	{0x2B, "Illegal", HuC6280::NO_OPERANDS},
	{0x2C, "BIT",     HuC6280::OPERAND_ABS},
	{0x2D, "AND",     HuC6280::OPERAND_ABS},
	{0x2E, "ROL",     HuC6280::OPERAND_ABS},
	{0x2F, "BBR2",    HuC6280::OPERAND_ZP_REL},
	{0x30, "BMI",     HuC6280::OPERAND_REL},
	{0x31, "AND",     HuC6280::OPERAND_INDY},
	{0x32, "AND",     HuC6280::OPERAND_IND},
	{0x33, "Illegal", HuC6280::NO_OPERANDS},
	{0x34, "BIT",     HuC6280::OPERAND_ZPX},
	{0x35, "AND",     HuC6280::OPERAND_ZPX},
	{0x36, "ROL",     HuC6280::OPERAND_ZPX},
	{0x37, "RMB3",    HuC6280::OPERAND_ZP},
	{0x38, "SEC",     HuC6280::NO_OPERANDS},
	{0x39, "AND",     HuC6280::OPERAND_ABSY},
	{0x3A, "Illegal", HuC6280::NO_OPERANDS},
	{0x3B, "Illegal", HuC6280::NO_OPERANDS},
	{0x3C, "BIT",     HuC6280::OPERAND_ABSX},
	{0x3D, "AND",     HuC6280::OPERAND_ABSX},
	{0x3E, "ROL",     HuC6280::OPERAND_ABSX},
	{0x3F, "BBR3",    HuC6280::OPERAND_ZP_REL},
	{0x40, "RTI",     HuC6280::NO_OPERANDS},
	{0x41, "EOR",     HuC6280::OPERAND_INDX},
	{0x42, "SAY",     HuC6280::NO_OPERANDS},
	{0x43, "TMA",     HuC6280::NO_OPERANDS},
	{0x44, "BSR",     HuC6280::OPERAND_REL},
	{0x45, "EOR",     HuC6280::OPERAND_ZP},
	{0x46, "LSR",     HuC6280::OPERAND_ZP},
	{0x47, "RMB4",    HuC6280::OPERAND_ZP},
	{0x48, "PHA",     HuC6280::NO_OPERANDS},
	{0x49, "EOR",     HuC6280::OPERAND_IMM},
	{0x4A, "LSR",     HuC6280::OPERAND_ACCUM},
	{0x4B, "Illegal", HuC6280::NO_OPERANDS},
	{0x4C, "JMP",     HuC6280::OPERAND_ABS},
	{0x4D, "EOR",     HuC6280::OPERAND_ABS},
	{0x4E, "LSR",     HuC6280::OPERAND_ZP},
	{0x4F, "BBR4",    HuC6280::OPERAND_ZP_REL},
	{0x50, "BVC",     HuC6280::OPERAND_REL},
	{0x51, "EOR",     HuC6280::OPERAND_INDY},
	{0x52, "EOR",     HuC6280::OPERAND_IND},
	{0x53, "TAM",     HuC6280::NO_OPERANDS},
	{0x54, "CSL",     HuC6280::NO_OPERANDS},
	{0x55, "EOR",     HuC6280::OPERAND_ZPX},
	{0x56, "LSR",     HuC6280::OPERAND_ZPX},
	{0x57, "RMB5",    HuC6280::OPERAND_ZP},
	{0x58, "CLI",     HuC6280::NO_OPERANDS},
	{0x59, "EOR",     HuC6280::OPERAND_ABSY},
	{0x5A, "PHY",     HuC6280::NO_OPERANDS},
	{0x5B, "Illegal", HuC6280::NO_OPERANDS},
	{0x5C, "Illegal", HuC6280::NO_OPERANDS},
	{0x5D, "EOR",     HuC6280::OPERAND_ABSX},
	{0x5E, "LSR",     HuC6280::OPERAND_ABSX},
	{0x5F, "BBR5",    HuC6280::OPERAND_ZP_REL},
	{0x60, "RTS",     HuC6280::NO_OPERANDS},
	{0x61, "ADC",     HuC6280::OPERAND_INDX},
	{0x62, "CLA",     HuC6280::NO_OPERANDS},
	{0x63, "Illegal", HuC6280::NO_OPERANDS},
	{0x64, "STZ",     HuC6280::OPERAND_ZP},
	{0x65, "ADC",     HuC6280::OPERAND_ZP},
	{0x66, "ROR",     HuC6280::OPERAND_ZP},
	{0x67, "RMB6",    HuC6280::OPERAND_ZP},
	{0x68, "PLA",     HuC6280::NO_OPERANDS},
	{0x69, "ADC",     HuC6280::OPERAND_IMM},
	{0x6A, "ROR",     HuC6280::OPERAND_ACCUM},
	{0x6B, "Illegal", HuC6280::NO_OPERANDS},
	{0x6C, "JMP",     HuC6280::OPERAND_ABSIND},
	{0x6D, "ADC",     HuC6280::OPERAND_ABS},
	{0x6E, "ROR",     HuC6280::OPERAND_ABS},
	{0x6F, "BBR6",    HuC6280::OPERAND_ZP_REL},
	{0x70, "BVS",     HuC6280::OPERAND_REL},
	{0x71, "ADC",     HuC6280::OPERAND_INDY},
	{0x72, "ADC",     HuC6280::OPERAND_IND},
	{0x73, "TII",     HuC6280::OPERAND_ABS_ABS_ABS},
	{0x74, "STZ",     HuC6280::OPERAND_ZPX},
	{0x75, "ADC",     HuC6280::OPERAND_ZPX},
	{0x76, "ROR",     HuC6280::OPERAND_ZPX},
	{0x77, "RMB7",    HuC6280::OPERAND_ZP},
	{0x78, "SEI",     HuC6280::NO_OPERANDS},
	{0x79, "ADC",     HuC6280::OPERAND_ABSY},
	{0x7A, "PLY",     HuC6280::NO_OPERANDS},
	{0x7B, "Illegal", HuC6280::NO_OPERANDS},
	{0x7C, "JMP",     HuC6280::OPERAND_ABSXIND},
	{0x7D, "ADC",     HuC6280::OPERAND_ABSX},
	{0x7E, "ROR",     HuC6280::OPERAND_ABSX},
	{0x7F, "BBR7",    HuC6280::OPERAND_ZP_REL},
	{0x80, "BRA",     HuC6280::OPERAND_ABSX},
	{0x81, "STA",     HuC6280::OPERAND_ZPX},
	{0x82, "CLX",     HuC6280::NO_OPERANDS},
	{0x83, "TST",     HuC6280::OPERAND_IMM_ZP},
	{0x84, "STY",     HuC6280::OPERAND_ZP},
	{0x85, "STA",     HuC6280::OPERAND_ZP},
	{0x86, "STX",     HuC6280::OPERAND_ZP},
	{0x87, "SMB0",    HuC6280::OPERAND_ZP},
	{0x88, "DEY",     HuC6280::NO_OPERANDS},
	{0x89, "BIT",     HuC6280::OPERAND_IMM},
	{0x8A, "TXA",     HuC6280::NO_OPERANDS},
	{0x8B, "Illegal", HuC6280::NO_OPERANDS},
	{0x8C, "STY",     HuC6280::OPERAND_ABS},
	{0x8D, "STA",     HuC6280::OPERAND_ABS},
	{0x8E, "STX",     HuC6280::OPERAND_ABS},
	{0x8F, "BBS0",    HuC6280::OPERAND_ZP_REL},
	{0x90, "BCC",     HuC6280::OPERAND_REL},
	{0x91, "STA",     HuC6280::OPERAND_INDY},
	{0x92, "STA",     HuC6280::OPERAND_IND},
	{0x93, "TST",     HuC6280::OPERAND_IMM_ABS},
	{0x94, "STY",     HuC6280::OPERAND_ZPX},
	{0x95, "STA",     HuC6280::OPERAND_ZPX},
	{0x96, "STX",     HuC6280::OPERAND_ZPY},
	{0x97, "SMB1",    HuC6280::OPERAND_ZP},
	{0x98, "TYA",     HuC6280::NO_OPERANDS},
	{0x99, "STA",     HuC6280::OPERAND_ABSY},
	{0x9A, "TXS",     HuC6280::NO_OPERANDS},
	{0x9B, "Illegal", HuC6280::NO_OPERANDS},
	{0x9C, "STZ",     HuC6280::OPERAND_ABS},
	{0x9D, "STA",     HuC6280::OPERAND_ABSX},
	{0x9E, "STZ",     HuC6280::OPERAND_ABSX},
	{0x9F, "BBS1",    HuC6280::OPERAND_ZP_REL},
	{0xA0, "LDY",     HuC6280::OPERAND_IMM},
	{0xA1, "LDA",     HuC6280::OPERAND_INDX},
	{0xA2, "LDX",     HuC6280::OPERAND_IMM},
	{0xA3, "TST",     HuC6280::OPERAND_IMM_ZPX},
	{0xA4, "LDY",     HuC6280::OPERAND_ZP},
	{0xA5, "LDA",     HuC6280::OPERAND_ZP},
	{0xA6, "LDX",     HuC6280::OPERAND_ZP},
	{0xA7, "SMB2",    HuC6280::OPERAND_ZP},
	{0xA8, "TAY",     HuC6280::NO_OPERANDS},
	{0xA9, "LDA",     HuC6280::OPERAND_IMM},
	{0xAA, "TAX",     HuC6280::NO_OPERANDS},
	{0xAB, "Illegal", HuC6280::NO_OPERANDS},
	{0xAC, "LDY",     HuC6280::OPERAND_ABS},
	{0xAD, "LDA",     HuC6280::OPERAND_ABS},
	{0xAE, "LDX",     HuC6280::OPERAND_ABS},
	{0xAF, "BBS2",    HuC6280::OPERAND_ZP_REL},
	{0xB0, "BCS",     HuC6280::OPERAND_REL},
	{0xB1, "LDA",     HuC6280::OPERAND_INDY},
	{0xB2, "LDA",     HuC6280::OPERAND_IND},
	{0xB3, "TST",     HuC6280::OPERAND_IMM_ABSX},
	{0xB4, "LDY",     HuC6280::OPERAND_ZPX},
	{0xB5, "LDA",     HuC6280::OPERAND_ZPX},
	{0xB6, "LDX",     HuC6280::OPERAND_ZPY},
	{0xB7, "SMB3",    HuC6280::OPERAND_ZP},
	{0xB8, "CLV",     HuC6280::NO_OPERANDS},
	{0xB9, "LDA",     HuC6280::OPERAND_ABSY},
	{0xBA, "TSX",     HuC6280::NO_OPERANDS},
	{0xBB, "TSX",     HuC6280::NO_OPERANDS},
	{0xBC, "LDY",     HuC6280::OPERAND_ABSX},
	{0xBD, "LDA",     HuC6280::OPERAND_ABSX},
	{0xBE, "LDX",     HuC6280::OPERAND_ABSY},
	{0xBF, "BBS3",    HuC6280::OPERAND_ZP_REL},
	{0xC0, "CPY",     HuC6280::OPERAND_IMM},
	{0xC1, "CMP",     HuC6280::OPERAND_INDX},
	{0xC2, "CLY",     HuC6280::NO_OPERANDS},
	{0xC3, "TDD",     HuC6280::OPERAND_ABS_ABS_ABS},
	{0xC4, "CPY",     HuC6280::OPERAND_ZP},
	{0xC5, "CMP",     HuC6280::OPERAND_ZP},
	{0xC6, "DEC",     HuC6280::OPERAND_ZP},
	{0xC7, "SMB4",    HuC6280::OPERAND_ZP},
	{0xC8, "INY",     HuC6280::NO_OPERANDS},
	{0xC9, "CMP",     HuC6280::OPERAND_IMM},
	{0xCA, "DEX",     HuC6280::NO_OPERANDS},
	{0xCB, "Illegal", HuC6280::NO_OPERANDS},
	{0xCC, "CPY",     HuC6280::OPERAND_ABS},
	{0xCD, "CMP",     HuC6280::OPERAND_ABS},
	{0xCE, "DEC",     HuC6280::OPERAND_ABS},
	{0xCF, "BBS4",    HuC6280::OPERAND_ZP_REL},
	{0xD0, "BNE",     HuC6280::OPERAND_REL},
	{0xD1, "CMP",     HuC6280::OPERAND_INDY},
	{0xD2, "CMP",     HuC6280::OPERAND_IND},
	{0xD3, "TIN",     HuC6280::OPERAND_ABS_ABS_ABS},
	{0xD4, "CSH",     HuC6280::NO_OPERANDS},
	{0xD5, "CMP",     HuC6280::OPERAND_ZPX},
	{0xD6, "DEC",     HuC6280::OPERAND_ZPX},
	{0xD7, "SMB5",    HuC6280::OPERAND_ZP},
	{0xD8, "CLD",     HuC6280::NO_OPERANDS},
	{0xD9, "CMP",     HuC6280::OPERAND_ABSY},
	{0xDA, "PHX",     HuC6280::NO_OPERANDS},
	{0xDB, "Illegal", HuC6280::NO_OPERANDS},
	{0xDC, "Illegal", HuC6280::NO_OPERANDS},
	{0xDD, "CMP",     HuC6280::OPERAND_ABSX},
	{0xDE, "DEC",     HuC6280::OPERAND_ABSX},
	{0xDF, "BBS5",    HuC6280::OPERAND_ZP_REL},
	{0xE0, "CPX",     HuC6280::OPERAND_IMM},
	{0xE1, "SBC",     HuC6280::OPERAND_INDX},
	{0xE2, "Illegal", HuC6280::NO_OPERANDS},
	{0xE3, "TIA",     HuC6280::OPERAND_ABS_ABS_ABS},
	{0xE4, "CPX",     HuC6280::OPERAND_ZP},
	{0xE5, "SBC",     HuC6280::OPERAND_ZP},
	{0xE6, "INC",     HuC6280::OPERAND_ZP},
	{0xE7, "SMB6",    HuC6280::OPERAND_ZP},
	{0xE8, "INX",     HuC6280::NO_OPERANDS},
	{0xE9, "SBC",     HuC6280::OPERAND_IMM},
	{0xEA, "NOP",     HuC6280::NO_OPERANDS},
	{0xEB, "Illegal", HuC6280::NO_OPERANDS},
	{0xEC, "CPX",     HuC6280::OPERAND_ABS},
	{0xED, "SBC",     HuC6280::OPERAND_ABS},
	{0xEE, "INC",     HuC6280::OPERAND_ABS},
	{0xEF, "BBS6",    HuC6280::OPERAND_ZP_REL},
	{0xF0, "BEQ",     HuC6280::OPERAND_REL},
	{0xF1, "SBC",     HuC6280::OPERAND_INDY},
	{0xF2, "SBC",     HuC6280::OPERAND_IND},
	{0xF3, "TAI",     HuC6280::OPERAND_ABS_ABS_ABS},
	{0xF4, "SET",     HuC6280::NO_OPERANDS},
	{0xF5, "SBC",     HuC6280::OPERAND_ZPX},
	{0xF6, "INC",     HuC6280::OPERAND_ZPX},
	{0xF7, "SMB7",    HuC6280::OPERAND_ZP},
	{0xF8, "SED",     HuC6280::NO_OPERANDS},
	{0xF9, "SBC",     HuC6280::OPERAND_ABSY},
	{0xFA, "PLX",     HuC6280::NO_OPERANDS},
	{0xFB, "Illegal", HuC6280::NO_OPERANDS},
	{0xFC, "Illegal", HuC6280::NO_OPERANDS},
	{0xFD, "SBC",     HuC6280::OPERAND_ABSX},
	{0xFE, "INC",     HuC6280::OPERAND_ABSX},
	{0xFF, "BBS7",    HuC6280::OPERAND_ZP_REL},
};

const uint16_t HuC6280Psg::HUC6280_VOL_TB[] = {
	// for (i : 0..15) YM_VOL_TB[i] = floor(power(10, (i-15)/6.67)*3840)
	21, 30, 43, 60, 86, 121, 171, 242, 342, 483, 683, 965, 1363, 1925, 2718, 3840
};


void HuC6280::Reset()
{
	mBrkVector = 0xfffe;	// ToDo: is this the correct BRK vector for the 6280?
	mRegs.F = 0;
	mTimer.Reset();
	mTimer.SetCpu(this);
}


void HuC6280::Irq(uint16_t vector) {
	if (!mRegs.F & HuC6280::FLAG_I) {
		PUSHW(mRegs.PC);
		PUSHB(mRegs.F);
		mRegs.F |= (HuC6280::FLAG_I);
		mRegs.PC = mMemory->ReadByte(vector);
		mRegs.PC |= (uint16_t)mMemory->ReadByte(vector+1) << 8;
		NLOGD(NLOG_TAG, "Irq(%#x) -> %#x", vector, mRegs.PC);
	}
}


void HuC6280::Run(uint32_t maxCycles)
{
	uint16_t addr, addr2, temp16;
	uint8_t operand, temp8;
    int8_t relAddr;

    NLOGD("HuC6280", "Run(%d)", maxCycles);

	while (mCycles < maxCycles) {

		//Disassemble(mRegs.PC);

		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);

		switch (opcode) {
		case 0x00:	// BRK
			mRegs.PC++;
			PUSHW(mRegs.PC);
			PUSHB(mRegs.F);
			mRegs.F &= ~(HuC6280::FLAG_D | HuC6280::FLAG_T);
			mRegs.F |= (HuC6280::FLAG_I);
			mRegs.PC = mMemory->ReadByte(mBrkVector);
			mRegs.PC |= (uint16_t)mMemory->ReadByte(mBrkVector+1) << 8;
			mCycles += 8;
			break;
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
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 3;
			break;
		case 0x03:	// ST0 imm
			operand = mMemory->ReadByte(mRegs.PC++);
			// ToDo: write the operand to the VDC address register
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x04:	// TSB zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_V | HuC6280::FLAG_N | HuC6280::FLAG_T);
			mRegs.F |= (operand & 0xC0);
			operand |= mRegs.A;
			mRegs.F |= (operand == 0) ? HuC6280::FLAG_Z : 0;
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x05:	// ORA zp
			operand = mMemory->ReadByte(ZP_ADDR());
			ORA(operand);
			mCycles += 4;
			break;
		case 0x06:	// ASL zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x07:	// RMB0 zp
			RMB(0);
			break;
		case 0x08:	// PHP
			PUSHB(mRegs.F);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 3;
			break;
		case 0x09:	// ORA imm
			operand = mMemory->ReadByte(mRegs.PC++);
			ORA(operand);
			mCycles += 2;
			break;
		case 0x0A:	// ASL A
			ASL(mRegs.A);
			mCycles += 2;
			break;
		// 0x0B is unused
		case 0x0C:	// TSB abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_V | HuC6280::FLAG_N | HuC6280::FLAG_T);
			mRegs.F |= (operand & 0xC0);
			operand |= mRegs.A;
			mRegs.F |= (operand == 0) ? HuC6280::FLAG_Z : 0;
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x0D:	// ORA abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			ORA(operand);
			mCycles += 5;
			break;
		case 0x0E:	// ASL abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x0F:	// BBR0 zp,rel
			BRANCH_ON_BIT(0, 0);
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
		case 0x12:	// ORA (zp)
			IND_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ORA(operand);
			mCycles += 7;
			break;
		case 0x13:	// ST1 imm
			operand = mMemory->ReadByte(mRegs.PC++);
			// ToDo: write the operand to the VDC low data register
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x14:	// TRB zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_V | HuC6280::FLAG_N | HuC6280::FLAG_T);
			mRegs.F |= (operand & 0xC0);
			operand &= ~mRegs.A;
			mRegs.F |= (operand == 0) ? HuC6280::FLAG_Z : 0;
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x15:	// ORA zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			ORA(operand);
			mCycles += 4;
			break;
		case 0x16:	// ASL zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x17:	// RMB1 zp
			RMB(1);
			break;
		case 0x18:	// CLC
			mRegs.F &= ~(HuC6280::FLAG_C | HuC6280::FLAG_T);
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
			mCycles += 2;
			break;
		// 0x1B is unused
		case 0x1C:	// TRB abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			mRegs.F &= ~(HuC6280::FLAG_Z | HuC6280::FLAG_V | HuC6280::FLAG_N | HuC6280::FLAG_T);
			mRegs.F |= (operand & 0xC0);
			operand &= ~mRegs.A;
			mRegs.F |= (operand == 0) ? HuC6280::FLAG_Z : 0;
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x1D:	// ORA abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ORA(operand);
			mCycles += 5;
			break;
		case 0x1E:	// ASL abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x1F:	// BBR1 zp,rel
			BRANCH_ON_BIT(1, 0);
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
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 3;
			break;
		case 0x23:	// ST2 imm
			operand = mMemory->ReadByte(mRegs.PC++);
			// ToDo: write the operand to the VDC high data register
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
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
		case 0x26:	// ROL zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x27:	// RMB2 zp
			RMB(2);
			break;
		case 0x28:	// PLP
			PULLB(mRegs.F);
			mCycles += 4;
			break;
		case 0x29:	// AND imm
			operand = mMemory->ReadByte(mRegs.PC++);
			AND(operand);
			mCycles += 2;
			break;
		case 0x2A:	// ROL A
			ROL(mRegs.A);
			mCycles += 2;
			break;
		// 0x2B is unused
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
		case 0x2E:	// ROL abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x2F:	// BBR2 zp,rel
			BRANCH_ON_BIT(2, 0);
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
		case 0x32:	// AND (zp)
			IND_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			AND(operand);
			mCycles += 7;
			break;
		// 0x33 is unused
		case 0x34:	// BIT zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			BIT(operand);
			mCycles += 4;
			break;
		case 0x35:	// AND zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			AND(operand);
			mCycles += 4;
			break;
		case 0x36:	// ROL zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x37:	// RMB3 zp
			RMB(3);
			break;
		case 0x38:	// SEC
			mRegs.F &= ~HuC6280::FLAG_T;
			mRegs.F |= HuC6280::FLAG_C;
			mCycles += 2;
			break;
		case 0x39:	// AND abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			AND(operand);
			mCycles += 5;
			break;
		// 0x3A is unused
		// 0x3B is unused
		case 0x3C:	// BIT abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			BIT(operand);
			mCycles += 5;
			break;
		case 0x3D:	// AND abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			AND(operand);
			mCycles += 5;
			break;
		case 0x3E:	// ROL abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x3F:	// BBR3 zp,rel
			BRANCH_ON_BIT(3, 0);
			break;

		case 0x40:	// RTI
			PULLB(mRegs.F);
			PULLB(mRegs.PC);
			PULLB(addr);
			mRegs.PC += (addr << 8);
			mCycles += 7;
			break;
		case 0x41:	// EOR (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 7;
			break;
		case 0x42:	// SAY
			temp8 = mRegs.A;
			mRegs.A = mRegs.Y;
			mRegs.Y = temp8;
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 3;
			break;
		case 0x43:	// TMA
			operand = mMemory->ReadByte(mRegs.PC++);
			if (operand) {
				for (int i = 0; i < 8; i++) {
					if (operand & (1 << i)) {
						mRegs.A = mMPR[i];
						break;
					}
				}
			} else {
				mRegs.A = mMprLatch;
			}
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
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
		case 0x46:	// LSR zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x47:	// RMB4 zp
			RMB(4);
			break;
		case 0x48:	// PHA
			PUSHB(mRegs.A);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 3;
			break;
		case 0x49:	// EOR imm
			operand = mMemory->ReadByte(mRegs.PC++);
			EOR(operand);
			mCycles += 2;
			break;
		case 0x4A:	// LSR A
			LSR(mRegs.A);
			mCycles += 2;
			break;
		// 0x4B is unused
		case 0x4C:	// JMP abs
			temp16 = mRegs.PC - 1;
			addr = mMemory->ReadByte(mRegs.PC++);
			addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			mRegs.PC = addr;
			if (addr == temp16) {
				// Special case for HES files; exit early for infinite loops
				return;
			}
			mCycles += 4;
			break;
		case 0x4D:	// EOR abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			EOR(operand);
			mCycles += 5;
			break;
		case 0x4E:	// LSR abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x4F:	// BBR4 zp,rel
			BRANCH_ON_BIT(4, 0);
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
		case 0x52:	// EOR (zp)
			IND_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 7;
			break;
		case 0x53:	// TAM
			operand = mMemory->ReadByte(mRegs.PC++);
			for (int i = 0; i < 8; i++) {
				if (operand & (1 << i)) {
					mMPR[i] = mRegs.A;
					mMprLatch = mRegs.A;
					mMemory->SetMpr(i, mRegs.A);
				}
			}
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x54:	// CSL
			// ToDo: tell player that the CPU speed has changed
			if (mSpeed) {
				maxCycles >>= 1;
			}
			mRegs.F &= ~HuC6280::FLAG_T;
			mSpeed = 0;
			mCycles += 3;
			break;
		case 0x55:	// EOR zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			EOR(operand);
			mCycles += 4;
			break;
		case 0x56:	// LSR zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x57:	// RMB5 zp
			RMB(5);
			break;
		case 0x58:	// CLI
			mRegs.F &= ~(HuC6280::FLAG_I | HuC6280::FLAG_T);
			mCycles += 2;
			break;
		case 0x59:	// EOR abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 5;
			break;
		case 0x5A:	// PHY
			PUSHB(mRegs.Y);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 3;
			break;
		// 0x5B is unused
		// 0x5C is unused
		case 0x5D:	// EOR abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			EOR(operand);
			mCycles += 5;
			break;
		case 0x5E:	// LSR abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x5F:	// BBR5 zp,rel
			BRANCH_ON_BIT(5, 0);
			break;

		case 0x60:	// RTS
			PULLB(mRegs.PC);
			PULLB(addr);
			mRegs.PC += (addr << 8) + 1;
			mCycles += 7;
			break;
		case 0x61:	// ADC (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 7;
			break;
		case 0x62:	// CLA
			mRegs.A = 0;
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		// 0x63 is unused
		case 0x64:	// STZ zp
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, 0);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x65:	// ADC zp
			operand = mMemory->ReadByte(ZP_ADDR());
			ADC(operand);
			mCycles += 4;
			break;
		case 0x66:	// ROR zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x67:	// RMB6 zp
			RMB(6);
			break;
		case 0x68:	// PLA
			PULLB(mRegs.A);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;
		case 0x69:	// ADC imm
			operand = mMemory->ReadByte(mRegs.PC++);
			ADC(operand);
			mCycles += 2;
			break;
		case 0x6A:	// ROR A
			ROR(mRegs.A);
			mCycles += 2;
			break;
		// 0x6B is unused
		case 0x6C:	// JMP (abs)
			addr = ABS_ADDR();
			mRegs.PC = mMemory->ReadByte(addr);
			mRegs.PC |= (uint16_t)mMemory->ReadByte((addr & 0xFF00) + ((addr+1) & 0xFF)) << 8;
			mCycles += 7;
			break;
		case 0x6D:	// ADC abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			ADC(operand);
			mCycles += 5;
			break;
		case 0x6E:	// ROR abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x6F:	// BBR6 zp,rel
			BRANCH_ON_BIT(6, 0);
			break;

		case 0x70:	// BVS rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_V);
			break;
		case 0x71:	// ADC (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 7;
			break;
		case 0x72:	// ADC (zp)
			IND_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 7;
			break;
		case 0x73:	// TII dddd,ssss,llll
			BLOCK_TRANSFER(1, 1, 0, 0);	// destInc=1, srcInc=1, destAlt=0, srcAlt=0
			break;
		case 0x74:	// STZ zp,X
			addr = ZPX_ADDR();
			mMemory->WriteByte(addr, 0);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x75:	// ADC zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			ADC(operand);
			mCycles += 4;
			break;
		case 0x76:	// ROR zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0x77:	// RMB7 zp
			RMB(7);
			break;
		case 0x78:	// SEI
			mRegs.F &= ~HuC6280::FLAG_T;
			mRegs.F |= HuC6280::FLAG_I;
			mCycles += 2;
			break;
		case 0x79:	// ADC abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 5;
			break;
		case 0x7A:	// PLY
			PULLB(mRegs.Y);
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;
		// 0x7B is unused
		case 0x7C:	// JMP (abs,X)
			ABSX_ADDR(addr);
			mRegs.PC = mMemory->ReadByte(addr);
			mRegs.PC |= (uint16_t)mMemory->ReadByte((addr & 0xFF00) + ((addr+1) & 0xFF)) << 8;
			mCycles += 7;
			break;
		case 0x7D:	// ADC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 5;
			break;
		case 0x7E:	// ROR abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0x7F:	// BBR7 zp,rel
			BRANCH_ON_BIT(7, 0);
			break;

		case 0x80:	// BRA rel
			COND_BRANCH(true);
			break;
		case 0x81:	// STA (zp,X)
			INDX_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 7;
			break;
		case 0x82:	// CLX
			mRegs.X = 0;
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		case 0x83:	// TST imm,zp
			temp8 = mMemory->ReadByte(mRegs.PC++);
			operand = mMemory->ReadByte(ZP_ADDR());
			TST(operand, temp8);
			mCycles += 7;
			break;
		case 0x84:	// STY zp
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, mRegs.Y);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x85:	// STA zp
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, mRegs.A);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x86:	// STX zp
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, mRegs.X);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x87:	// SMB0 zp
			SMB(0);
			break;
		case 0x88:	// DEY
			mRegs.Y--;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;
		case 0x89:	// BIT imm
			operand = mMemory->ReadByte(mRegs.PC++);
			BIT(operand);
			mCycles += 2;
			break;
		case 0x8A:	// TXA
			mRegs.A = mRegs.X;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		// 0x8B is unused
		case 0x8C:	// STY abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			mMemory->WriteByte(addr, mRegs.Y);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x8D:	// STA abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			mMemory->WriteByte(addr, mRegs.A);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x8E:	// STX abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			mMemory->WriteByte(addr, mRegs.X);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x8F:	// BBS0 zp,rel
			BRANCH_ON_BIT(0, 1);
			break;

		case 0x90:	// BCC rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_C) == 0);
			break;
		case 0x91:	// STA (zp),Y
			INDY_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 7;
			break;
		case 0x92:	// STA (zp)
			IND_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 7;
			break;
		case 0x93:	// TST imm,abs
			temp8 = mMemory->ReadByte(mRegs.PC++);
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			TST(operand, temp8);
			mCycles += 8;
			break;
		case 0x94:	// STY zp,X
			addr = ZPX_ADDR();
			mMemory->WriteByte(addr, mRegs.Y);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x95:	// STA zp,X
			addr = ZPX_ADDR();
			mMemory->WriteByte(addr, mRegs.A);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x96:	// STX zp,Y
			addr = ZPY_ADDR();
			mMemory->WriteByte(addr, mRegs.X);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 4;
			break;
		case 0x97:	// SMB1 zp
			SMB(1);
			break;
		case 0x98:	// TYA
			mRegs.A = mRegs.Y;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0x99:	// STA abs,Y
			ABSY_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x9A:	// TXS
			mRegs.S = mRegs.X;
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		// 0x9B is unused
		case 0x9C:	// STZ abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			mMemory->WriteByte(addr, 0);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x9D:	// STA abs,X
			ABSX_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x9E:	// STZ abs,X
			ABSX_ADDR(addr);
			mMemory->WriteByte(addr, 0);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 5;
			break;
		case 0x9F:	// BBS1 zp,rel
			BRANCH_ON_BIT(1, 1);
			break;


		case 0xA0:	// LDY imm
			mRegs.Y = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;
		case 0xA1:	// LDA (zp,X)
			INDX_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 7;
			break;
		case 0xA2:	// LDX imm
			mRegs.X = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		case 0xA3:	// TST imm,zp,X
			temp8 = mMemory->ReadByte(mRegs.PC++);
			operand = mMemory->ReadByte(ZPX_ADDR());
			TST(operand, temp8);
			mCycles += 7;
			break;
		case 0xA4:	// LDY zp
			mRegs.Y = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;
		case 0xA5:	// LDA zp
			mRegs.A = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;
		case 0xA6:	// LDX zp
			mRegs.X = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;
		case 0xA7:	// SMB2 zp
			SMB(2);
			break;
		case 0xA8:	// TAY
			mRegs.Y = mRegs.A;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;
		case 0xA9:	// LDA imm
			mRegs.A = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;
		case 0xAA:	// TAX
			mRegs.X = mRegs.A;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		// 0xAB is unused
		case 0xAC:	// LDY abs
			mRegs.Y = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.Y);
			mCycles += 5;
			break;
		case 0xAD:	// LDA abs
			mRegs.A = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;
		case 0xAE:	// LDX abs
			mRegs.X = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.X);
			mCycles += 5;
			break;
		case 0xAF:	// BBS2 zp,rel
			BRANCH_ON_BIT(2, 1);
			break;


		case 0xB0:	// BCS rel
			COND_BRANCH(mRegs.F & HuC6280::FLAG_C);
			break;
		case 0xB1:	// LDA (zp),Y
			INDY_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 7;
			break;
		case 0xB2:	// LDA (zp)
			IND_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 7;
			break;
		case 0xB3:	// TST imm,abs,X
			temp8 = mMemory->ReadByte(mRegs.PC++);
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			TST(operand, temp8);
			mCycles += 8;
			break;
		case 0xB4:	// LDY zp,X
			mRegs.Y = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;
		case 0xB5:	// LDA zp,X
			mRegs.A = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;
		case 0xB6:	// LDX zp,Y
			mRegs.X = mMemory->ReadByte(ZPY_ADDR());
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;
		case 0xB7:	// SMB3 zp
			SMB(3);
			break;
		case 0xB8:	// CLV
			mRegs.F &= ~(HuC6280::FLAG_V | HuC6280::FLAG_T);
			mCycles += 2;
			break;
		case 0xB9:	// LDA abs,Y
			ABSY_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;
		case 0xBA:	// TSX
			mRegs.X = mRegs.S;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		// 0xBB is unused
		case 0xBC:	// LDY abs,X
			ABSX_ADDR(addr);
			mRegs.Y = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.Y);
			mCycles += 5;
			break;
		case 0xBD:	// LDA abs,X
			ABSX_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;
		case 0xBE:	// LDX abs,Y
			ABSY_ADDR(addr);
			mRegs.X = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.X);
			mCycles += 5;
			break;
		case 0xBF:	// BBS3 zp,rel
			BRANCH_ON_BIT(3, 1);
			break;


		case 0xC0:	// CPY imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.Y, operand, operand);
			mCycles += 2;
			break;
		case 0xC1:	// CMP (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 7;
			break;
		case 0xC2:	// CLY
			mRegs.Y = 0;
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 2;
			break;
		case 0xC3:	// TDD dddd,ssss,llll
			BLOCK_TRANSFER(-1, -1, 0, 0);	// destInc=-1, srcInc=-1, destAlt=0, srcAlt=0
			break;
		case 0xC4:	// CPY zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.Y, operand, operand);
			mCycles += 4;
			break;
		case 0xC5:	// CMP zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 4;
			break;
		case 0xC6:	// DEC zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xC7:	// SMB4 zp
			SMB(4);
			break;
		case 0xC8:	// INY
			mRegs.Y++;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;
		case 0xC9:	// CMP imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 2;
			break;
		case 0xCA:	// DEX
			mRegs.X--;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		// 0xCB is unused
		case 0xCC:	// CPY abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.Y, operand, operand);
			mCycles += 5;
			break;
		case 0xCD:	// CMP abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 5;
			break;
		case 0xCE:	// DEC abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0xCF:	// BBS4 zp,rel
			BRANCH_ON_BIT(4, 1);
			break;


		case 0xD0:	// BNE rel
			COND_BRANCH((mRegs.F & HuC6280::FLAG_Z) == 0);
			break;
		case 0xD1:	// CMP (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 7;
			break;
		case 0xD2:	// CMP (zp)
			IND_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 7;
			break;
		case 0xD3:	// TIN dddd,ssss,llll
			BLOCK_TRANSFER(0, 1, 0, 0);	// destInc=0, srcInc=1, destAlt=0, srcAlt=0
			break;
		case 0xD4:	// CSH
			// ToDo: tell player that the CPU speed has changed
			if (!mSpeed) {
				maxCycles <<= 1;
			}
			mRegs.F &= ~HuC6280::FLAG_T;
			mSpeed = 1;
			mCycles += 3;
			break;
		case 0xD5:	// CMP zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 4;
			break;
		case 0xD6:	// DEC zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xD7:	// SMB5 zp
			SMB(5);
			break;
		case 0xD8:	// CLD
			mRegs.F &= ~(HuC6280::FLAG_D | HuC6280::FLAG_T);
			mCycles += 2;
			break;
		case 0xD9:	// CMP abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 5;
			break;
		case 0xDA:	// PHX
			PUSHB(mRegs.X);
			mRegs.F &= ~HuC6280::FLAG_T;
			mCycles += 3;
			break;
		// 0xDB is unused
		// 0xDC is unused
		case 0xDD:	// CMP abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand, operand);
			mCycles += 5;
			break;
		case 0xDE:	// DEC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0xDF:	// BBS5 zp,rel
			BRANCH_ON_BIT(5, 1);
			break;


		case 0xE0:	// CPX imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.X, operand, operand);
			mCycles += 2;
			break;
		case 0xE1:	// SBC (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 7;
			break;
		// 0xE2 is unused
		case 0xE3:	// TIA dddd,ssss,llll
			BLOCK_TRANSFER(0, 1, 1, 0);	// destInc=0, srcInc=1, destAlt=1, srcAlt=0
			break;
		case 0xE4:	// CPX zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.X, operand, operand);
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
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xE7:	// SMB6 zp
			SMB(6);
			break;
		case 0xE8:	// INX
			mRegs.X++;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;
		case 0xE9:	// SBC imm
			operand = mMemory->ReadByte(mRegs.PC++) ^ 0xFF;
			ADC(operand);
			mCycles += 2;
			break;
		case 0xEA:	// NOP
			mCycles += 2;
			break;
		// 0xEB is unused
		case 0xEC:	// CPX abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.X, operand, operand);
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
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0xEF:	// BBS6 zp,rel
			BRANCH_ON_BIT(6, 1);
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
		case 0xF3:	// TAI dddd,ssss,llll
			BLOCK_TRANSFER(1, 0, 0, 1);	// destInc=1, srcInc=0, destAlt=0, srcAlt=1
			break;
		case 0xF4:	// SET
			mRegs.F |= HuC6280::FLAG_T;
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
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;
		case 0xF7:	// SMB7 zp
			SMB(7);
			break;
		case 0xF8:	// SED
			mRegs.F &= ~HuC6280::FLAG_T;
			mRegs.F |= HuC6280::FLAG_D;
			mCycles += 2;
			break;
		case 0xF9:	// SBC abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 5;
			break;
		case 0xFA:	// PLX
			PULLB(mRegs.X);
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;
		// 0xFB is unused
		// 0xFC is unused
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
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;
		case 0xFF:	// BBS7 zp,rel
			BRANCH_ON_BIT(7, 1);
			break;

		default:
			ILLEGAL_OP();
			break;
		}
	}
	NLOGD(NLOG_TAG, "Done, mCycles = %d", mCycles);
}


void HuC6280::Disassemble(uint16_t address)
{
	uint8_t opcode = mMemory->ReadByte(address);
	uint8_t operand1, operand2;
	std::string opcodeStr, machineCodeStr, eaStr;
	char temp[16];
	char operandStr[20];
	operand1 = mMemory->ReadByte(address+1);
	operand2 = mMemory->ReadByte(address+2);

	snprintf(temp, 16, "%02x", opcode);
	machineCodeStr = temp;

	opcodeStr = gDisassemblyTable[opcode].mnemonic;

	switch(gDisassemblyTable[opcode].operands) {
	case OPERAND_ACCUM:
		snprintf(operandStr, 16, " A");
		break;
	case OPERAND_IMM:
		snprintf(operandStr, 16, " #$%02x", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;
	case OPERAND_ZP:
		snprintf(operandStr, 16, " $%02x", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;
	case OPERAND_ZPX:
		snprintf(operandStr, 16, " $%02x,X", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		snprintf(temp, 16, " [%#x]", (operand1 + mRegs.X) & 0xFF);
		eaStr = temp;
		break;
	case OPERAND_ZPY:
		snprintf(operandStr, 16, " $%02x,Y", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		snprintf(temp, 16, " [%#x]", (operand1 + mRegs.Y) & 0xFF);
		eaStr = temp;
		break;
	case OPERAND_ABS:
		snprintf(operandStr, 16, " $%02x%02x", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		break;
	case OPERAND_ABSX:
		snprintf(operandStr, 16, " $%02x%02x,X", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		break;
	case OPERAND_ABSY:
		snprintf(operandStr, 16, " $%02x%02x,Y", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		break;
	case OPERAND_IND:
		snprintf(operandStr, 16, " ($%02x)", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;
	case OPERAND_INDX:
		snprintf(operandStr, 16, " ($%02x,X)", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;
	case OPERAND_INDY:
		snprintf(operandStr, 16, " ($%02x),Y", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;
	case OPERAND_ABSIND:
		// ToDo: handle
		break;
	case OPERAND_ABSXIND:
		// ToDo: handle
		break;
	case OPERAND_REL:
		snprintf(operandStr, 16, " <$%04x", address + 2 + (int8_t)operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;
	case OPERAND_ZP_REL:
		// ToDo: handle
		break;
	case OPERAND_IMM_ABS:
		// ToDo: handle
		break;
	case OPERAND_IMM_ABSX:
		// ToDo: handle
		break;
	case OPERAND_IMM_ZP:
		// ToDo: handle
		break;
	case OPERAND_IMM_ZPX:
		// ToDo: handle
		break;
	case OPERAND_ABS_ABS_ABS:
		snprintf(operandStr, 16, " $%02x%02x", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		// ToDo: handle all 3 operands
		break;
	case NO_OPERANDS:
	default:
		snprintf(operandStr, 16, "");
		break;
	}

	NLOGD("HuC6280", "0x%04x: %s %s%s%s | A=%#x, X=%#x, Y=%#x, F=%#x, SP=%#x (cyc=%d)",
			address, machineCodeStr.c_str(), opcodeStr.c_str(), operandStr, eaStr.c_str(), mRegs.A, mRegs.X, mRegs.Y, mRegs.F, mRegs.S, mCycles);
}


void HuC6280Timer::Reset()
{
	mPos = mCtrl = 0;
	mStep = 0;
	mCycles = 0;
}


void HuC6280Timer::Step()
{
	if (mCtrl && HuC6280Timer::CTRL_STARTED) {
		mStep++;
		if (mStep >= mPeriod) {
			mStep = 0;
			mCycles += mPeriod;
			mPos = (mPos - 1) & 0x7F;
			if (0x7F == mPos) {
				m6280->mMemory->Irq(HuC6280Mapper::TIMER_IRQ);
				mCycles = 0;
			}
		}
	}
}


HuC6280PsgChannel::HuC6280PsgChannel()
	: mVolL(0), mVolR(0)
	, mEnable(HuC6280Psg::WRITE_WAVEFORM_RAM)
	, mWaveReadPos(0), mWaveWritePos(0)
{
}

void HuC6280PsgChannel::Reset()
{
	// ToDo: implement
	mVolL = mVolR = 0;
	mEnable = HuC6280Psg::WRITE_WAVEFORM_RAM;
}

void HuC6280PsgChannel::Step()
{
	// ToDo: implement
	if ((mEnable & HuC6280Psg::WRITE_MODE) == HuC6280Psg::CHN_ENABLE) {
		mPos++;
		if (mPos >= mPeriod) {
			mPeriod = 0;
			mOut = mWaveformRam[mWaveReadPos++];
			mWaveReadPos &= 0x1F;
		}
	}
}

void HuC6280PsgChannel::Write(uint32_t addr, uint8_t data)
{
	switch (addr) {
	case HuC6280Psg::R_FREQ_LO:
		mPeriod = (mPeriod & 0xF00) | data;
		break;
	case HuC6280Psg::R_FREQ_HI:
		mPeriod = (mPeriod & 0xFF) | ((uint16_t)(data & 0x0F) << 8);
		break;
	case HuC6280Psg::R_CHN_BALANCE:
		mVolR = data & 0x0F;
		mVolL = data >> 4;
		break;
	case HuC6280Psg::R_WAVE_DATA:
		switch (mEnable & HuC6280Psg::WRITE_MODE) {
		case HuC6280Psg::WRITE_WAVEFORM_RAM:
			mWaveformRam[mWaveWritePos++] = data & 0x1F;
			mWaveWritePos &= 0x1F;
			break;
		case HuC6280Psg::DDA_ENABLE:
			mOut = data & 0x1F;
			break;
		default:
			break;	// Ignore the write
		}
		break;
	default:
		break;
	}
}

HuC6280Psg::HuC6280Psg()
	: mChannelSelect(0)
{
}

void HuC6280Psg::Reset()
{
	for (int i = 0; i < 6; i++) {
		mChannels[i].Reset();
	}
	mMasterVolL = mMasterVolR = 0;
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
	NLOGD(NLOG_TAG, "Write(%#x, %#x)", addr, data);

	switch (addr) {
	case R_CHN_SELECT:
		if (data < 6) {
			mChannelSelect = data;
		}
		break;
	case R_BALANCE:
		mMasterVolR = data & 0x0F;
		mMasterVolL = data >> 4;
		mPlayer->SetMasterVolume(mMasterVolL, mMasterVolR);
		break;
	case R_FREQ_LO:
	case R_FREQ_HI:
	case R_ENABLE:
	case R_CHN_BALANCE:
	case R_WAVE_DATA:
		mChannels[mChannelSelect].Write(addr, data);
		break;
	case R_LFO_FREQ:
		// ToDo: handle
		break;
	case R_LFO_CTRL:
		// ToDo: handle
		break;
	default:
		break;
	}
}
