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

#include <FBase.h>
#include "Emu6502.h"

#define UPDATE_NZ(val) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_N); \
				       mRegs.F |= (val == 0) ? Emu6502::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80)

#define UPDATE_NZC(val, val2) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_N) | Emu6502::FLAG_C; \
				       mRegs.F |= (val == 0) ? Emu6502::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80); \
				       mRegs.F |= (val >= val2) ? FLAG_C : 0

#define ABS_ADDR() (mMemory->ReadByte(mRegs.PC) + ((uint16_t)mMemory->ReadByte(mRegs.PC+1) << 8))

#define ZP_ADDR() (mMemory->ReadByte(mRegs.PC++))

#define ZPX_ADDR() ((ZP_ADDR() + mRegs.X) & 0xFF)

#define ILLEGAL_OP() AppLog("Emu6502::Run: Illegal opcode: %#x at PC=%#x", opcode, mRegs.PC); \
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

	while (mCycles < maxCycles) {
		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);
		switch (opcode) {
		case 0x02: case 0x03: case 0x04:
			ILLEGAL_OP();
			break;

		case 0x05:		// ORA zp
			mRegs.A |= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0x09:		// ORA imm
			mRegs.A |= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x12: case 0x13: case 0x14:
			ILLEGAL_OP();
			break;

		case 0x15:		// ORA zp,X
			mRegs.A |= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x18:		// CLC
			mRegs.F &= ~Emu6502::FLAG_C;
			mCycles += 2;
			break;

		case 0x1A: case 0x1B: case 0x1C:
			ILLEGAL_OP();
			break;

		case 0x25:		// AND zp
			mRegs.A &= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0x29:		// AND imm
			mRegs.A &= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x2D:		// AND abs
			mRegs.A &= mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x32: case 0x33: case 0x34:
			ILLEGAL_OP();
			break;

		case 0x35:		// AND zp,X
			mRegs.A &= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x38:		// SEC
			mRegs.F |= Emu6502::FLAG_C;
			mCycles += 2;
			break;

		case 0x42: case 0x43: case 0x44:
			ILLEGAL_OP();
			break;

		case 0x45:		// EOR zp
			mRegs.A ^= mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 3;
			break;

		case 0x49:		// EOR imm
			mRegs.A ^= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0x4C:		// JMP abs
			 addr = mMemory->ReadByte(mRegs.PC++);
			 addr |= (uint16_t)mMemory->ReadByte(mRegs.PC) << 8;
			 mRegs.PC = addr;
			 mCycles += 3;
			 break;

		case 0x52: case 0x53: case 0x54:
			ILLEGAL_OP();
			break;

		case 0x55:		// EOR zp,X
			mRegs.A ^= mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZ(mRegs.A);
			mCycles += 4;
			break;

		case 0x58:		// CLI
			mRegs.F &= ~Emu6502::FLAG_I;
			mCycles += 2;
			break;

		case 0x5A: case 0x5B: case 0x5C:
			ILLEGAL_OP();
			break;

		case 0x62: case 0x63: case 0x64:
			ILLEGAL_OP();
			break;

		case 0x72: case 0x73: case 0x74:
			ILLEGAL_OP();
			break;

		case 0x78:		// SEI
			mRegs.F |= Emu6502::FLAG_I;
			mCycles += 2;
			break;

		case 0x7A: case 0x7B: case 0x7C:
			ILLEGAL_OP();
			break;

		case 0x80:
			ILLEGAL_OP();
			break;

		case 0x82: case 0x83:
			ILLEGAL_OP();
			break;

		case 0x88:		// DEY
			mRegs.Y--;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0x89:
			ILLEGAL_OP();
			break;

		case 0x8B:
			ILLEGAL_OP();
			break;

		case 0x8F:
			ILLEGAL_OP();
			break;

		case 0x92: case 0x93:
			ILLEGAL_OP();
			break;

		case 0x97:
			ILLEGAL_OP();
			break;

		case 0x9B: case 0x9C:
			ILLEGAL_OP();
			break;

		case 0x9E: case 0x9F:
			ILLEGAL_OP();
			break;

		case 0xA3:
			ILLEGAL_OP();
			break;

		case 0xA7:
			ILLEGAL_OP();
			break;

		case 0xA9:		// LDA imm
			mRegs.A = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0xAB:
			ILLEGAL_OP();
			break;

		case 0xAF:
			ILLEGAL_OP();
			break;

		case 0xB2: case 0xB3:
			ILLEGAL_OP();
			break;

		case 0xB7:
			ILLEGAL_OP();
			break;

		case 0xB8:		// CLV
			mRegs.F &= ~Emu6502::FLAG_V;
			mCycles += 2;
			break;

		case 0xBB:
			ILLEGAL_OP();
			break;

		case 0xBF:
			ILLEGAL_OP();
			break;

		case 0xC0:		// CPY imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.Y, operand);
			mCycles += 2;
			break;

		case 0xC2: case 0xC3:
			ILLEGAL_OP();
			break;

		case 0xC4:		// CPY zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.Y, operand);
			mCycles += 3;
			break;

		case 0xC5:		// CMP zp
			operand = mMemory->ReadByte(ZP_ADDR());
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 3;
			break;

		case 0xC6:		// DEC zp
			addr = ZP_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 5;
			break;

		case 0xC7:
			ILLEGAL_OP();
			break;

		case 0xC8:		// INY
			mRegs.Y++;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0xC9:		// CMP imm
			operand = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 2;
			break;

		case 0xCA:		// DEX
			mRegs.X--;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0xCB:
			ILLEGAL_OP();
			break;

		case 0xCC:		// CPY abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.Y, operand);
			mCycles += 4;
			break;

		case 0xCE:		// DEC abs
			addr = ABS_ADDR();
			mRegs.PC += 2;
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0xCF:
			ILLEGAL_OP();
			break;

		case 0xD2: case 0xD3: case 0xD4:
			ILLEGAL_OP();
			break;

		case 0xD5:		// CMP zp,X
			operand = mMemory->ReadByte(ZPX_ADDR());
			UPDATE_NZC(mRegs.A, operand);
			mCycles += 4;
			break;

		case 0xD6:		// DEC zp,X
			addr = ZPX_ADDR();
			operand = mMemory->ReadByte(addr) - 1;
			UPDATE_NZ(operand);
			mMemory->WriteByte(addr, operand);
			mCycles += 6;
			break;

		case 0xD7:
			ILLEGAL_OP();
			break;

		case 0xDA: case 0xDB: case 0xDC:
			ILLEGAL_OP();
			break;

		case 0xDF:
			ILLEGAL_OP();
			break;

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

		case 0xE8:		// INX
			mRegs.X++;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0xEA:		// NOP
			mCycles += 2;
			break;

		case 0xEC:		// CPX abs
			operand = mMemory->ReadByte(ABS_ADDR());
			mRegs.PC += 2;
			UPDATE_NZC(mRegs.X, operand);
			mCycles += 4;
			break;

		case 0xF2: case 0xF3: case 0xF4:
			ILLEGAL_OP();
			break;
		}
	}
}
