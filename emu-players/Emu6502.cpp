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
#include "Emu6502.h"

#define UPDATE_NZ(val) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_N); \
				       mRegs.F |= ((uint8_t)val == 0) ? Emu6502::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80)

#define UPDATE_NZC(val, val2) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_N | Emu6502::FLAG_C); \
				       mRegs.F |= ((uint8_t)val == (uint8_t)val2) ? Emu6502::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80); \
				       mRegs.F |= (val >= val2) ? FLAG_C : 0

// == Arithmetic ==

#define ADC(val) temp16 = (uint16_t)(val) + (uint16_t)mRegs.A + ((mRegs.F & Emu6502::FLAG_C) ? 1 : 0); \
				 UPDATE_NZC(temp16, 0x100); \
				 mRegs.F &= ~Emu6502::FLAG_V; \
				 mRegs.F |= ((!((mRegs.A ^ val) & 0x80)) && ((mRegs.A ^ temp16) & 0x80)) ? Emu6502::FLAG_V : 0; \
				 mRegs.A = (uint8_t)temp16

#define ASL(val) mRegs.F &= ~Emu6502::FLAG_C; \
	             mRegs.F |= (val & 0x80) ? Emu6502::FLAG_C : 0; \
	             val <<= 1; \
	             UPDATE_NZ(val)

#define BIT(val) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_V | Emu6502::FLAG_N); \
				 mRegs.F |= (((uint8_t)val & mRegs.A) == 0) ? Emu6502::FLAG_Z : 0; \
				 mRegs.F |= (val & 0xC0)

#define LSR(val) mRegs.F &= ~Emu6502::FLAG_C; \
	             mRegs.F |= (val & 0x01) ? Emu6502::FLAG_C : 0; \
	             val >>= 1; \
	             UPDATE_NZ(val)

#define ROL(val) temp8 = (val << 1) | ((mRegs.F & Emu6502::FLAG_C) ? 1 : 0); \
				 mRegs.F &= ~Emu6502::FLAG_C; \
	             mRegs.F |= (val & 0x80) ? Emu6502::FLAG_C : 0; \
	             val = temp8; \
	             UPDATE_NZ(val)

#define ROR(val) temp8 = (val >> 1) | ((mRegs.F & Emu6502::FLAG_C) ? 0x80 : 0); \
				 mRegs.F &= ~Emu6502::FLAG_C; \
	             mRegs.F |= (val & 0x01) ? Emu6502::FLAG_C : 0; \
	             val = temp8; \
	             UPDATE_NZ(val)

// == Addressing ==

// abs
// Doesn't update PC
#define ABS_ADDR() (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8))

// abs,X
// Updates PC
#define ABSX_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    if ((dest & 0x100) != ((dest + mRegs.X) & 0x100)) mCycles++; \
	                    mRegs.PC += 2; \
						dest += mRegs.X

// abs,Y
// Updates PC
#define ABSY_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    if ((dest & 0x100) != ((dest + mRegs.Y) & 0x100)) mCycles++; \
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

// (zp,X)
// Updates PC
#define INDX_ADDR(dest) dest = ZPX_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte(dest+1) << 8))

// (zp),Y
// Updates PC
#define INDY_ADDR(dest) dest = ZP_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte(dest+1) << 8)); \
						if ((dest & 0x100) != ((dest + mRegs.Y) & 0x100)) mCycles++; \
						dest += mRegs.Y


// ====

#define COND_BRANCH(cc) if ((cc)) { relAddr = mMemory->ReadByte(mRegs.PC++); \
								    addr = mRegs.PC + relAddr; \
                                    mCycles += ((addr & 0x100) == (mRegs.PC & 0x100)) ? 1 : 2; \
                                    mRegs.PC = addr; } else { mRegs.PC++; } \
                                  mCycles += 2

#define PUSHB(val) mMemory->WriteByte(0x100 + (uint16_t)mRegs.S, val); \
	               mRegs.S--

#define PUSHW(val) mMemory->WriteByte(0x100 + (uint16_t)mRegs.S, ((uint16_t)val >> 8)); \
	               mRegs.S--; \
	               mMemory->WriteByte(0x100 + (uint16_t)mRegs.S, (uint8_t)val); \
	               mRegs.S--

#define PULLB(dest) mRegs.S++; \
	                dest = mMemory->ReadByte(0x100 + (uint16_t)mRegs.S)


#define ILLEGAL_OP() NLOGE("Emu6502", "Run(): Illegal opcode: %#x at PC=%#x", opcode, mRegs.PC); \
					 mCycles += 2; \
					 mRegs.PC++

// ====


void Emu6502::Reset()
{
	mBrkVector = 0xfffe;
}

void Emu6502::Run(uint32_t maxCycles)
{
	uint16_t addr, temp16;
	uint8_t operand, temp8;
    int8_t relAddr;

	while (mCycles < maxCycles) {
		//Disassemble(mRegs.PC);

		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);
		/*NLOGD("Emu6502", "Emu6502::Run: op %#x, PC %#x, A %#x, X %#x, Y %#x, F %#x",
				  opcode, mRegs.PC-1, mRegs.A, mRegs.X, mRegs.Y, mRegs.F);*/

		switch (opcode) {

// == ADC ==
		case 0x69:		// ADC imm
			operand = mMemory->ReadByte(mRegs.PC++);
			ADC(operand);
			mCycles += 2;
			break;

		case 0x65:		// ADC zp
			operand = mMemory->ReadByte(ZP_ADDR());
			ADC(operand);
			mCycles += 3;
			break;

		case 0x75:		// ADC zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			ADC(operand);
			mCycles += 4;
			break;

		case 0x6D:		// ADC abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			ADC(operand);
			mCycles += 4;
			break;

		case 0x7D:		// ADC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 4;
			break;

		case 0x79:		// ADC abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 4;
			break;

		case 0x61:		// ADC (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 6;
			break;

		case 0x71:		// ADC (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ADC(operand);
			mCycles += 5;
			break;


// == AND ==
		case 0x29:		// AND imm
			mRegs.A &= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x25:		// AND zp
			mRegs.A &= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0x35:		// AND zp,X
			mRegs.A &= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x2D:		// AND abs
			mRegs.A &= mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x3D:		// AND abs,X
			ABSX_ADDR(addr);
			mRegs.A &= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x39:		// AND abs,Y
			ABSY_ADDR(addr);
			mRegs.A &= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x21:		// AND (zp,X)
			INDX_ADDR(addr);
			mRegs.A &= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 6;
			break;

		case 0x31:		// AND (zp),Y
			INDY_ADDR(addr);
			mRegs.A &= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;


// == ASL ==
		case 0x0A:		// ASL A
			ASL(mRegs.A);
			mCycles += 2;
			break;

		case 0x06:		// ASL zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x16:		// ASL zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x0E:		// ASL abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x1E:		// ASL abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ASL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;


// == Bxx ==
		case 0x10:		// BPL rel
			COND_BRANCH((mRegs.F & Emu6502::FLAG_N) == 0);
			break;
		case 0x30:		// BMI rel
			COND_BRANCH(mRegs.F & Emu6502::FLAG_N);
			break;

		case 0x50:		// BVC rel
			COND_BRANCH((mRegs.F & Emu6502::FLAG_V) == 0);
			break;
		case 0x70:		// BVS rel
			COND_BRANCH(mRegs.F & Emu6502::FLAG_V);
			break;

		case 0x90:		// BCC rel
			COND_BRANCH((mRegs.F & Emu6502::FLAG_C) == 0);
			break;
		case 0xB0:		// BCS rel
			COND_BRANCH(mRegs.F & Emu6502::FLAG_C);
			break;

		case 0xD0:		// BNE rel
			COND_BRANCH((mRegs.F & Emu6502::FLAG_Z) == 0);
			break;
		case 0xF0:		// BEQ rel
			COND_BRANCH(mRegs.F & Emu6502::FLAG_Z);
			break;

// == BIT ==
		case 0x24:		// BIT zp
			operand = mMemory->ReadByte(ZP_ADDR());
			BIT(operand);
			mCycles += 3;
			break;

		case 0x2C:		// BIT abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			BIT(operand);
			mCycles += 4;
			break;


// ====
		case 0x00:		// BRK
			mRegs.PC++;
			PUSHW(mRegs.PC);
			temp8 = mRegs.F | (Emu6502::FLAG_B | 0x20);
			PUSHB(temp8);
			mRegs.F |= (Emu6502::FLAG_B | Emu6502::FLAG_I);
			mRegs.PC = mMemory->ReadByte(mBrkVector);
			mRegs.PC |= (uint16_t)mMemory->ReadByte(mBrkVector+1) << 8;
			mCycles += 7;
			break;


// == CLx ==
		case 0x18:		// CLC
			mRegs.F &= ~Emu6502::FLAG_C;
			mCycles += 2;
			break;

		case 0xD8:		// CLD
			mRegs.F &= ~Emu6502::FLAG_D;
			mCycles += 2;
			break;

		case 0x58:		// CLI
			mRegs.F &= ~Emu6502::FLAG_I;
			mCycles += 2;
			break;

		case 0xB8:		// CLV
			mRegs.F &= ~Emu6502::FLAG_V;
			mCycles += 2;
			break;


// == CMP ==
		case 0xC9:		// CMP imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 2;
			break;

		case 0xC5:		// CMP zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 3;
			break;

		case 0xD5:		// CMP zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 4;
			break;

		case 0xCD:		// CMP abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 4;
			break;

		case 0xDD:		// CMP abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 4;
			break;

		case 0xD9:		// CMP abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 4;
			break;

		case 0xC1:		// CMP (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 6;
			break;

		case 0xD1:		// CMP (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 5;
			break;

// == CPX ==
		case 0xE0:		// CPX imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.X, operand);
			mCycles += 2;
			break;

		case 0xE4:		// CPX zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.X, operand);
			mCycles += 3;
			break;

		case 0xEC:		// CPX abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.X, operand);
			mCycles += 4;
			break;

// == CPY ==
		case 0xC0:		// CPY imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.Y, operand);
			mCycles += 2;
			break;

		case 0xC4:		// CPY zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.Y, operand);
			mCycles += 3;
			break;

		case 0xCC:		// CPY abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.Y, operand);
			mCycles += 4;
			break;


// == DEC ==
		case 0xC6:		// DEC zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0xD6:		// DEC zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0xCE:		// DEC abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0xDE:		// DEC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

// ====
		case 0xCA:		// DEX
			mRegs.X--;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0x88:		// DEY
			mRegs.Y--;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;


// == EOR ==
		case 0x49:		// EOR imm
			mRegs.A ^= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x45:		// EOR zp
			mRegs.A ^= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0x55:		// EOR zp,X
			mRegs.A ^= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x4D:		// EOR abs
			mRegs.A ^= mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x5D:		// EOR abs,X
			ABSX_ADDR(addr);
			mRegs.A ^= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x59:		// EOR abs,Y
			ABSY_ADDR(addr);
			mRegs.A ^= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x41:		// EOR (zp,X)
			INDX_ADDR(addr);
			mRegs.A ^= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 6;
			break;

		case 0x51:		// EOR (zp),Y
			INDY_ADDR(addr);
			mRegs.A ^= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 6;
			break;

// == INC ==
		case 0xE6:		// INC zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0xF6:		// INC zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0xEE:		// INC abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0xFE:		// INC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) + 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

// ====
		case 0xE8:		// INX
			mRegs.X++;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0xC8:		// INY
			mRegs.Y++;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

// ====
		case 0x4C:		// JMP abs
			temp16 = mRegs.PC - 1;
			addr = mMemory->ReadByte(mRegs.PC++);
			addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			mRegs.PC = addr;
			if (addr == temp16) {
				// Special case for NSFs; exit early for infinite loops
				return;
			}
			mCycles += 3;
			break;

		case 0x6C:		// JMP (abs)
			addr = ABS_ADDR();
			mRegs.PC = mMemory->ReadByte(addr);
			mRegs.PC |= (uint16_t)mMemory->ReadByte(addr+1) << 8;
			mCycles += 5;
			break;

		case 0x20:		// JSR abs
			 addr = mMemory->ReadByte(mRegs.PC++);
			 addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			 PUSHW(mRegs.PC);
			 mRegs.PC = addr;
			 mCycles += 6;
			 break;


// == LDA ==
		case 0xA9:		// LDA imm
			mRegs.A = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0xA5:		// LDA zp
			mRegs.A = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0xB5:		// LDA zp,X
			mRegs.A = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0xAD:		// LDA abs
			mRegs.A = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0xBD:		// LDA abs,X
			ABSX_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0xB9:		// LDA abs,Y
			ABSY_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0xA1:		// LDA (zp,X)
			INDX_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 6;
			break;

		case 0xB1:		// LDA (zp),Y
			INDY_ADDR(addr);
			mRegs.A = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;

// == LDX ==
		case 0xA2:		// LDX imm
			mRegs.X = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0xA6:		// LDX zp
			mRegs.X = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.X);
			mCycles += 3;
			break;

		case 0xB6:		// LDX zp,Y
			mRegs.X = mMemory->ReadByte(ZPY_ADDR());
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;

		case 0xAE:		// LDX abs
			mRegs.X = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;

		case 0xBE:		// LDX abs,Y
			ABSY_ADDR(addr);
			mRegs.X = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.X);
			mCycles += 4;
			break;


// == LDY ==
		case 0xA0:		// LDY imm
			mRegs.Y = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0xA4:		// LDY zp
			mRegs.Y = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.Y);
			mCycles += 3;
			break;

		case 0xB4:		// LDY zp,X
			mRegs.Y = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;

		case 0xAC:		// LDY abs
			mRegs.Y = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;

		case 0xBC:		// LDY abs,X
			ABSX_ADDR(addr);
			mRegs.Y = mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.Y);
			mCycles += 4;
			break;


// == LSR ==
		case 0x4A:		// LSR A
			LSR(mRegs.A);
			mCycles += 2;
			break;

		case 0x46:		// LSR zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x56:		// LSR zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x4E:		// LSR abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x5E:		// LSR abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			LSR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;


// == ORA ==
		case 0x09:		// ORA imm
			mRegs.A |= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x05:		// ORA zp
			mRegs.A |= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0x15:		// ORA zp,X
			mRegs.A |= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x0D:		// ORA abs
			mRegs.A |= mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x1D:		// ORA abs,X
			ABSX_ADDR(addr);
			mRegs.A |= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x19:		// ORA abs,Y
			ABSY_ADDR(addr);
			mRegs.A |= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x01:		// ORA (zp,X)
			INDX_ADDR(addr);
			mRegs.A |= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 6;
			break;

		case 0x11:		// ORA (zp),Y
			INDY_ADDR(addr);
			mRegs.A |= mMemory->ReadByte(addr);
			UPDATE_NZ(mRegs.A);
			mCycles += 5;
			break;

// == PHx ==
		case 0x48:		// PHA
			PUSHB(mRegs.A);
			mCycles += 3;
			break;

		case 0x08:		// PHP
			PUSHB(mRegs.F);
			mCycles += 3;
			break;

// == PLx ==
		case 0x68:		// PLA
			PULLB(mRegs.A);
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x28:		// PLP
			PULLB(mRegs.F);
			mCycles += 4;
			break;


// == ROL ==
		case 0x2A:		// ROL A
			ROL(mRegs.A);
			mCycles += 2;
			break;

		case 0x26:		// ROL zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x36:		// ROL zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x2E:		// ROL abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x3E:		// ROL abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ROL(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;

// == ROR ==
		case 0x6A:		// ROR A
			ROR(mRegs.A);
			mCycles += 2;
			break;

		case 0x66:		// ROR zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0x76:		// ROR zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x6E:		// ROR abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0x7E:		// ROR abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr);
			ROR(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 7;
			break;


// ====
		case 0x40:		// RTI
			PULLB(mRegs.F);
			PULLB(mRegs.PC);
			PULLB(addr);
			mRegs.PC += (addr << 8);
			mCycles += 6;
			break;

		case 0x60:		// RTS
			PULLB(mRegs.PC);
			PULLB(addr);
			mRegs.PC += (addr << 8) + 1;
			mCycles += 6;
			break;


// == SBC ==
		case 0xE9:		// SBC imm
			operand = mMemory->ReadByte(mRegs.PC++) ^ 0xFF;
			ADC(operand);
			mCycles += 2;
			break;

		case 0xE5:		// SBC zp
			operand = mMemory->ReadByte(ZP_ADDR()) ^ 0xFF;
			ADC(operand);
			mCycles += 3;
			break;

		case 0xF5:		// SBC zp,X
			operand = mMemory->ReadByte(ZPX_ADDR()) ^ 0xFF;
			ADC(operand);
			mCycles += 4;
			break;

		case 0xED:		// SBC abs
			operand = mMemory->ReadByte(ABS_ADDR()) ^ 0xFF;
			mRegs.PC += 2;
			ADC(operand);
			mCycles += 4;
			break;

		case 0xFD:		// SBC abs,X
			ABSX_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 4;
			break;

		case 0xF9:		// SBC abs,Y
			ABSY_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 4;
			break;

		case 0xE1:		// SBC (zp,X)
			INDX_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 6;
			break;

		case 0xF1:		// SBC (zp),Y
			INDY_ADDR(addr);
			operand = mMemory->ReadByte(addr) ^ 0xFF;
			ADC(operand);
			mCycles += 5;
			break;


// == SEx ==
		case 0x38:		// SEC
			mRegs.F |= Emu6502::FLAG_C;
			mCycles += 2;
			break;

		case 0xF8:		// SED
			mRegs.F |= Emu6502::FLAG_D;
			mCycles += 2;
			break;

		case 0x78:		// SEI
			mRegs.F |= Emu6502::FLAG_I;
			mCycles += 2;
			break;

// == STA ==
		case 0x85:		// STA zp
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 3;
			break;

		case 0x95:		// STA zp,X
			addr = ZPX_ADDR();
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 4;
			break;

		case 0x8D:		// STA abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 4;
			break;

		case 0x9D:		// STA abs,X
			ABSX_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 4;
			break;

		case 0x99:		// STA abs,Y
			ABSY_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 4;
			break;

		case 0x81:		// STA (zp,X)
			INDX_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 6;
			break;

		case 0x91:		// STA (zp),Y
			INDY_ADDR(addr);
			mMemory->WriteByte(addr, mRegs.A);
			mCycles += 5;
			break;


// == STX ==
		case 0x86:		// STX zp
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, mRegs.X);
			mCycles += 3;
			break;

		case 0x96:		// STX zp,Y
			addr = ZPY_ADDR();
			mMemory->WriteByte(addr, mRegs.X);
			mCycles += 4;
			break;

		case 0x8E:		// STX abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			mMemory->WriteByte(addr, mRegs.X);
			mCycles += 4;
			break;

// == STY ==
		case 0x84:		// STY zp
			addr = ZP_ADDR();
			mMemory->WriteByte(addr, mRegs.Y);
			mCycles += 3;
			break;

		case 0x94:		// STY zp,X
			addr = ZPX_ADDR();
			mMemory->WriteByte(addr, mRegs.Y);
			mCycles += 4;
			break;

		case 0x8C:		// STY abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			mMemory->WriteByte(addr, mRegs.Y);
			mCycles += 4;
			break;


// == Txx ==
		case 0xAA:		// TAX
			mRegs.X = mRegs.A;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0xA8:		// TAY
			mRegs.Y = mRegs.A;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0xBA:		// TSX
			mRegs.X = mRegs.S;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0x8A:		// TXA
			mRegs.A = mRegs.X;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x9A:		// TXS
			mRegs.S = mRegs.X;
			UPDATE_NZ(mRegs.S);
			mCycles += 2;
			break;

		case 0x98:		// TYA
			mRegs.A = mRegs.Y;
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

// ====
		case 0xEA:		// NOP
			mCycles += 2;
			break;

		default:
			ILLEGAL_OP();
			break;


		}
	}
}




void Emu6502::Disassemble(uint16_t address)
{
	uint8_t opcode = mMemory->ReadByte(address);
	uint8_t operand1, operand2;
	std::string opcodeStr, machineCodeStr;
	char temp[16];
	char operandStr[16];
	operand1 = mMemory->ReadByte(address+1);
	operand2 = mMemory->ReadByte(address+2);

	snprintf(temp, 16, "%02x", opcode);
	machineCodeStr = temp;

	switch (opcode) {
	case 0x69: case 0x65: case 0x75:
	case 0x6D: case 0x7D: case 0x79:
	case 0x61: case 0x71:
		opcodeStr = "ADC";
		break;

	case 0x29: case 0x25: case 0x35:
	case 0x2D: case 0x3D: case 0x39:
	case 0x21: case 0x31:
		opcodeStr = "AND";
		break;

	case 0x0A: case 0x06: case 0x16:
	case 0x0E: case 0x1E:
		opcodeStr = "ASL";
		break;

	case 0x10:
		opcodeStr = "BPL";
		break;
	case 0x30:
		opcodeStr = "BMI";
		break;
	case 0x50:
		opcodeStr = "BVC";
		break;
	case 0x70:
		opcodeStr = "BVS";
		break;
	case 0x90:
		opcodeStr = "BCC";
		break;
	case 0xB0:
		opcodeStr = "BCS";
		break;
	case 0xD0:
		opcodeStr = "BNE";
		break;
	case 0xF0:
		opcodeStr = "BEQ";
		break;

	case 0x24: case 0x2C:
		opcodeStr = "BIT";
		break;

	case 0x00:
		opcodeStr = "BRK";
		break;

	case 0x18:
		opcodeStr = "CLC";
		break;
	case 0xD8:
		opcodeStr = "CLD";
		break;
	case 0x58:
		opcodeStr = "CLI";
		break;
	case 0xB8:
		opcodeStr = "CLV";
		break;

	case 0xC9: case 0xC5: case 0xD5:
	case 0xCD: case 0xDD: case 0xD9:
	case 0xC1: case 0xD1:
		opcodeStr = "CMP";
		break;

	case 0xE0: case 0xE4: case 0xEC:
		opcodeStr = "CPX";
		break;

	case 0xC0: case 0xC4: case 0xCC:
		opcodeStr = "CPY";
		break;

	case 0xC6: case 0xD6: case 0xCE:
	case 0xDE:
		opcodeStr = "DEC";
		break;

	case 0xCA:
		opcodeStr = "DEX";
		break;

	case 0x88:
		opcodeStr = "DEY";
		break;

	case 0x49: case 0x45: case 0x55:
	case 0x4D: case 0x5D: case 0x59:
	case 0x41: case 0x51:
		opcodeStr = "EOR";
		break;

	case 0xE6: case 0xF6: case 0xEE:
	case 0xFE:
		opcodeStr = "INC";
		break;
	case 0xE8:
		opcodeStr = "INX";
		break;
	case 0xC8:
		opcodeStr = "INY";
		break;

	case 0x4C: case 0x6C:
		opcodeStr = "JMP";
		break;
	case 0x20:
		opcodeStr = "JSR";
		 break;

	case 0xA9: case 0xA5: case 0xB5:
	case 0xAD: case 0xBD: case 0xB9:
	case 0xA1: case 0xB1:
		opcodeStr = "LDA";
		break;

	case 0xA2: case 0xA6: case 0xB6:
	case 0xAE: case 0xBE:
		opcodeStr = "LDX";
		break;

	case 0xA0: case 0xA4: case 0xB4:
	case 0xAC: case 0xBC:
		opcodeStr = "LDY";
		break;

	case 0x4A: case 0x46: case 0x56:
	case 0x4E: case 0x5E:
		opcodeStr = "LSR";
		break;

	case 0x09: case 0x05: case 0x15:
	case 0x0D: case 0x1D: case 0x19:
	case 0x01: case 0x11:
		opcodeStr = "ORA";
		break;

	case 0x48:
		opcodeStr = "PHA";
		break;
	case 0x08:
		opcodeStr = "PHP";
		break;

	case 0x68:
		opcodeStr = "PLA";
		break;
	case 0x28:
		opcodeStr = "PLP";
		break;

	case 0x2A: case 0x26: case 0x36:
	case 0x2E: case 0x3E:
		opcodeStr = "ROL";
		break;

	case 0x6A: case 0x66: case 0x76:
	case 0x6E: case 0x7E:
		opcodeStr = "ROR";
		break;

	case 0x40:
		opcodeStr = "RTI";
		break;
	case 0x60:
		opcodeStr = "RTS";
		break;

	case 0xE9: case 0xE5: case 0xF5:
	case 0xED: case 0xFD: case 0xF9:
	case 0xE1: case 0xF1:
		opcodeStr = "SBC";
		break;

	case 0x38:
		opcodeStr = "SEC";
		break;
	case 0xF8:
		opcodeStr = "SED";
		break;
	case 0x78:
		opcodeStr = "SEI";
		break;

	case 0x85: case 0x95: case 0x8D:
	case 0x9D: case 0x99: case 0x81:
	case 0x91:
		opcodeStr = "STA";
		break;

	case 0x86: case 0x96: case 0x8E:
		opcodeStr = "STX";
		break;

	case 0x84: case 0x94: case 0x8C:
		opcodeStr = "STY";
		break;

	case 0xAA:
		opcodeStr = "TAX";
		break;
	case 0xA8:
		opcodeStr = "TAY";
		break;
	case 0xBA:
		opcodeStr = "TSX";
		break;
	case 0x8A:
		opcodeStr = "TXA";
		break;
	case 0x9A:
		opcodeStr = "TXS";
		break;
	case 0x98:
		opcodeStr = "TYA";
		break;

	case 0xEA:
		opcodeStr = "NOP";
		break;

	default:
		opcodeStr = "ILLEGAL";
		break;
	}

	switch (opcode) {
	case 0x09: case 0x29: case 0x49: case 0x69:
	case 0xA0: case 0xA2: case 0xA9: case 0xC0:
	case 0xC9: case 0xE0: case 0xE9:
		snprintf(operandStr, 16, " #$%02x", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;

	case 0x05: case 0x06: case 0x24: case 0x25:
	case 0x26: case 0x45: case 0x46: case 0x65:
	case 0x66: case 0x84: case 0x85: case 0x86:
	case 0xA4: case 0xA5: case 0xA6: case 0xC4:
	case 0xC5: case 0xC6: case 0xE4: case 0xE5:
	case 0xE6:
		snprintf(operandStr, 16, " $%02x", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;

	case 0x15: case 0x16: case 0x35: case 0x36:
	case 0x55: case 0x56: case 0x75: case 0x76:
	case 0x94: case 0x95: case 0xB4: case 0xB5:
	case 0xD5: case 0xD6: case 0xF5: case 0xF6:
		snprintf(operandStr, 16, " $%02x,X", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;

	case 0x96: case 0xB6:
		snprintf(operandStr, 16, " $%02x,Y", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;

	case 0x01: case 0x21: case 0x41: case 0x61:
	case 0x81: case 0xA1: case 0xC1: case 0xE1:
		snprintf(operandStr, 16, " ($%02x,X)", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;

	case 0x11: case 0x31: case 0x51: case 0x71:
	case 0x91: case 0xB1: case 0xD1: case 0xF1:
		snprintf(operandStr, 16, " ($%02x),Y", operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;

	case 0x0D: case 0x0E: case 0x2C: case 0x2D:
	case 0x2E: case 0x4C: case 0x4D: case 0x4E:
	case 0x6D: case 0x6E: case 0x8C: case 0x8D:
	case 0x8E: case 0xAC: case 0xAD: case 0xAE:
	case 0xCC: case 0xCD: case 0xCE: case 0xEC:
	case 0xED: case 0xEE: case 0x20:
		snprintf(operandStr, 16, " $%02x%02x", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		break;

	case 0x6C:
		snprintf(operandStr, 16, " ($%02x%02x)", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		break;

	case 0x1D: case 0x1E: case 0x3D: case 0x3E:
	case 0x5D: case 0x5E: case 0x7D: case 0x7E:
	case 0x9D: case 0xBC: case 0xBD: case 0xDD:
	case 0xDE: case 0xFD: case 0xFE:
		snprintf(operandStr, 16, " $%02x%02x,X", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		break;

	case 0x19: case 0x39: case 0x59: case 0x79:
	case 0x99: case 0xB9: case 0xBE: case 0xD9:
	case 0xF9:
		snprintf(operandStr, 16, " $%02x%02x,Y", operand2, operand1);
		snprintf(temp, 16, " %02x %02x", operand1, operand2);
		machineCodeStr += temp;
		break;

	case 0x10: case 0x30: case 0x50: case 0x70:
	case 0x90: case 0xB0: case 0xD0: case 0xF0:
		snprintf(operandStr, 16, " <$%04x", address + 2 + (int8_t)operand1);
		snprintf(temp, 8, " %02x", operand1);
		machineCodeStr += temp;
		break;

	default:
		snprintf(operandStr, 16, "");
		break;
	}

	NLOGD("Emu6502", "%#x: %s %s%s | A=%#x, X=%#x, Y=%#x, F=%#x, SP=%#x", address, machineCodeStr.c_str(), opcodeStr.c_str(), operandStr, mRegs.A, mRegs.X, mRegs.Y, mRegs.F, mRegs.S);
}

