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

#include "NativeLogger.h"
#include "Emu6502.h"

#define UPDATE_NZ(val) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_N); \
				       mRegs.F |= (val == 0) ? Emu6502::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80)

#define UPDATE_NZC(val, val2) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_N | Emu6502::FLAG_C); \
				       mRegs.F |= (val == 0) ? Emu6502::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80); \
				       mRegs.F |= (val >= val2) ? FLAG_C : 0

// Doesn't update PC
#define ABS_ADDR() (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8))

// Updates PC
#define ABSX_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    if ((dest & 0x100) != ((dest + mRegs.X) & 0x100)) mCycles++; \
	                    mRegs.PC += 2; \
						dest += mRegs.X

// Updates PC
#define ABSY_ADDR(dest) dest = (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8)); \
	                    if ((dest & 0x100) != ((dest + mRegs.Y) & 0x100)) mCycles++; \
	                    mRegs.PC += 2; \
						dest += mRegs.Y

// Updates PC
#define ZP_ADDR() (mMemory->ReadByte(mRegs.PC++))

// Updates PC
#define ZPX_ADDR() ((ZP_ADDR() + mRegs.X) & 0xFF)

// Updates PC
#define INDX_ADDR(dest) dest = ZPX_ADDR(); \
						dest = (mMemory->ReadByte(dest) + ((uint16_t)mMemory->ReadByte(dest+1) << 8));

#define COND_BRANCH(cc) if ((cc)) { relAddr = mMemory->ReadByte(mRegs.PC++); \
								    addr = mRegs.PC + relAddr; \
                                    mCycles += ((addr & 0x100) == (mRegs.PC & 0x100)) ? 1 : 2; \
                                    mRegs.PC = addr; } \
                                  mCycles += 2

#define ILLEGAL_OP() NativeLog(0, "Emu6502", "Run(): Illegal opcode: %#x at PC=%#x", opcode, mRegs.PC); \
					 mCycles += 2; \
					 mRegs.PC++

void Emu6502::Reset()
{
	// TODO: fill out
}


void Emu6502::Run(uint32_t maxCycles)
{
	uint16_t addr;
	uint8_t operand;
    int8_t relAddr;

	while (mCycles < maxCycles) {
		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);
		switch (opcode) {

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

// == ASL ==

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

// == CLx ==
		case 0x18:		// CLC
			mRegs.F &= ~Emu6502::FLAG_C;
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
			 addr = mMemory->ReadByte(mRegs.PC++);
			 addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			 mRegs.PC = addr;
			 mCycles += 3;
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


// == LDX ==
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

// == SEx ==
		case 0x38:		// SEC
			mRegs.F |= Emu6502::FLAG_C;
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
