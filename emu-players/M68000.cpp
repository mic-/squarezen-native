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
#include "M68000.h"


void M68000::Reset()
{
	// ToDo: implement
}

void M68000::Run(uint32_t maxCycles)
{
	uint8_t opcode, opcodeLo;
	uint16_t opcode16;
	uint32_t srcMode, dstMode;
	uint32_t srcReg, dstReg;

	// ToDo: implement

	opcode = mMemory->ReadByte(mRegs.PC++);
	opcodeLo = mMemory->ReadByte(mRegs.PC++);
	opcode16 = ((uint16_t)opcode << 8) | opcodeLo;

	switch (opcode & 0xF0) {
	case 0x30:
		dstMode = (opcode16 >> 6) & 7;
		srcMode = (opcodeLo >> 3) & 7;
		dstReg = (opcode >> 1) & 7;
		if (dstMode == 1) {
			// MOVEA
			switch (srcMode) {
			case 0x00:	// Dn
				mRegs.A[dstReg] = mRegs.D[opcodeLo & 7];
				// ToDo: update flags and cycles
				break;
			case 0x01:	// An
				mRegs.A[dstReg] = mRegs.A[opcodeLo & 7];
				// ToDo: update flags and cycles
				break;
			}
		} else {
			// MOVE
		}
		break;
	case 0x40:
		dstMode = (opcode16 >> 6) & 7;
		if (dstMode == 7) {
			// LEA
		}
		break;
	case 0x70:
		if (!(opcode & 1)) {
			// MOVEQ #imm8,Dn
			mRegs.D[(opcode >> 1) & 7] = (int8_t)opcodeLo;
			// ToDo: update flags and cycles
		}
		break;
	default:
		break;
	}
}
