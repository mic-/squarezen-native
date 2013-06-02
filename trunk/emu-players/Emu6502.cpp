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

#include "Emu6502.h"

#define UPDATE_NZ(val) mRegs.F &= ~(Emu6502::FLAG_Z | Emu6502::FLAG_N); \
				       mRegs.F |= (val == 0) ? Emu6502::FLAG_Z : 0; \
				       mRegs.F |= (val & 0x80)

#define ZP_ADDR() mMemory->ReadByte(mRegs.PC++)


void Emu6502::Run(uint32_t maxCycles)
{
	uint16_t addr;

	while (mCycles < maxCycles) {
		uint8_t opcode = mMemory->ReadByte(mRegs.PC++);
		switch (opcode) {
		case 0x09:		// ORA imm
			mRegs.A |= mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
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

		case 0x58:		// CLI
			mRegs.F &= ~Emu6502::FLAG_I;
			mCycles += 2;
			break;

		case 0x78:		// SEI
			mRegs.F |= Emu6502::FLAG_I;
			mCycles += 2;
			break;

		case 0x88:		// DEY
			mRegs.Y--;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0xA9:		// LDA imm
			mRegs.A = mMemory->ReadByte(mRegs.PC++);
			UPDATE_NZ(mRegs.A);
			mCycles += 2;
			break;

		case 0xB8:		// CLV
			mRegs.F &= ~Emu6502::FLAG_V;
			mCycles += 2;
			break;

		case 0xC8:		// INY
			mRegs.Y++;
			UPDATE_NZ(mRegs.Y);
			mCycles += 2;
			break;

		case 0xE8:		// INX
			mRegs.X++;
			UPDATE_NZ(mRegs.X);
			mCycles += 2;
			break;

		case 0xEA:		// NOP
			mCycles += 2;
			break;
		}
	}
}
