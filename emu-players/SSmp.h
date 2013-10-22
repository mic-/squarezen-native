/*
 * SSmp.h
 *
 *  Created on: Jul 8, 2013
 *
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

#ifndef SSMP_H_
#define SSMP_H_

#include <string>
#include <stdint.h>
#include "MemoryMapper.h"

class SSmp
{
public:
	void Reset();
	void Run(uint32_t maxCycles);
	void SetMapper(MemoryMapper *mapper) { mMemory = mapper; }

	typedef struct {
		uint8_t opcode;
		const std::string mnemonic;
		uint32_t operands;
	} Instruction;

	// Instruction operands
	enum {
		NO_OPERANDS,
		OPERAND_ACCUM,		// A
		OPERAND_PSW,		// PSW
		OPERAND_IMM,		// #nn
		OPERAND_ZP,			// aa
		OPERAND_ZPX,		// aa+X
		OPERAND_ZPY,		// aa+Y
		OPERAND_ABS,		// !aaaa
		OPERAND_ABSX,		// !aaaa+X
		OPERAND_ABSY,		// !aaaa+Y
		OPERAND_IND,
		OPERAND_INDX,		// [aa+X]
		OPERAND_INDY,		// [aa]+Y
		OPERAND_ABSIND,
		OPERAND_ABSXIND,
		OPERAND_REL,
		OPERAND_ZPBIT,		// aa.b
		OPERAND_ATX,		// (X)
	};

	enum {
		S_SMP_CLOCK = 2048000
	};

	enum {
		FLAG_C = 0x01,
		FLAG_Z = 0x02,
		FLAG_I = 0x04,
		FLAG_H = 0x08,
		FLAG_B = 0x10,
		FLAG_P = 0x20,
		FLAG_V = 0x40,
		FLAG_N = 0x80,
	};

	struct {
		uint8_t A, X, Y, SP, PSW;
		uint16_t PC;
	} mRegs;

	uint32_t mCycles;
	MemoryMapper *mMemory;
	bool mHalted;
};


#endif /* SSMP_H_ */
