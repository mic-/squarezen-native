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
#define NLOG_TAG "ARM7TDMI"

#include <string>
#include <stdio.h>
#include "NativeLogger.h"
#include "ARM7TDMI.h"

#define THUMB_GET_RS_RD(instr, Rs, Rd) \
	Rd = (instr) & 7; \
	Rs = (instr >> 3) & 7

#define THUMB_UPDATE_NZ(val32) \
	mCpsr |= val32 & FLAG_N; \
	mCpsr |= val32 ? 0 : FLAG_Z


static const ARM7TDMI::InstructionDecoder ARM_DECODER_TABLE[] =
{

};

static const ARM7TDMI::InstructionDecoder THUMB_DECODER_TABLE[] =
{
	&ARM7TDMI::ThumbType00,
	&ARM7TDMI::ThumbType01,
};


void ARM7TDMI::Reset()
{
	// ToDo: implement
}


inline void ARM7TDMI::DecodeARM(uint32_t instruction)
{
	// Decoder bits:
	// d27 d26 d25 d7 d6 d5 d4

	uint32_t decoderBits = (instruction >> 4) & 0x0F;
	decoderBits |= ((instruction >> 21) & 0x70);
	(this->*ARM_DECODER_TABLE[decoderBits])(instruction);
}


inline void ARM7TDMI::DecodeThumb(uint32_t instruction)
{
	// Decoder bits:
	// d15 d14 d13 d12 d11
	uint32_t decoderBits = (instruction >> 11) & 0x1F;
	(this->*THUMB_DECODER_TABLE[decoderBits])(instruction);
}


void ARM7TDMI::Run(uint32_t maxCycles)
{
	while (mCycles < maxCycles) {
		uint32_t instruction;
		if (mCpsr & ARM7TDMI::FLAG_T) {
			instruction = mMemory->ReadHalfWord(mRegs[15]);
			mRegs[15] += 2;
			DecodeThumb(instruction);
		} else {
			instruction = mMemory->ReadWord(mRegs[15]);
			mRegs[15] += 4;
			DecodeARM(instruction);
		}
	}
}



void ARM7TDMI::ThumbType00(uint32_t instruction)
{
	// LSL Rd,Rs,#Offset5

	uint32_t rs, rd, shamt;

	THUMB_GET_RS_RD(instruction, rs, rd);
	shamt = (instruction >> 6) & 0x1F;

	mCpsr &= ~(FLAG_C | FLAG_Z | FLAG_N);
	mRegs[rd] = mRegs[rs] << shamt;
	mCpsr |= (mRegs[rs] & (1 << (32 - shamt))) ? FLAG_C : 0;
	THUMB_UPDATE_NZ(mRegs[rd]);
	mCycles++;
}


void ARM7TDMI::ThumbType01(uint32_t instruction)
{
	// LSR Rd,Rs,#Offset5

	uint32_t rs, rd, shamt;

	THUMB_GET_RS_RD(instruction, rs, rd);
	shamt = (instruction >> 6) & 0x1F;

	mCpsr &= ~(FLAG_C | FLAG_Z | FLAG_N);
	if (shamt) {
		mRegs[rd] = mRegs[rs] >> shamt;
		mCpsr |= (mRegs[rs] & (1 << (shamt - 1))) ? FLAG_C : 0;
		THUMB_UPDATE_NZ(mRegs[rd]);
	} else {
		mRegs[rd] = 0;
		mCpsr |= (mRegs[rs] & (1 << 31)) ? FLAG_C : 0;
		mCpsr |= FLAG_Z;
	}
	mCycles++;
}


