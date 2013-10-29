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

static const ARM7TDMI::InstructionDecoder ARM_DECODER_TABLE[] =
{

};

static const ARM7TDMI::InstructionDecoder THUMB_DECODER_TABLE[] =
{
	&ARM7TDMI::ThumbType00,
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
		if (mCpsr & 0x20) {
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

	/*thumb_type_00:
		mov 		esi,eax
		mov 		ecx,eax
		mov 		edi,eax
		ADD_CYCLES 	1
		and 		esi,38h
		and 		ecx,7C0h
		shr 		esi,1
		and 		edi,7
		mov 		esi,[r0+esi]
		shr 		ecx,6
		jz 		thumb_type_00_zero
		shl 		esi,cl
		T_UPDATE_C_FORCE
		T_UPDATE_NZ
		mov 		[r0+edi*4],esi
		T_RETURN
	thumb_type_00_zero:
		and 		esi,esi
		T_UPDATE_NZ
		mov 		[r0+edi*4],esi
		;ADD_CYCLES 	1
		T_RETURN*/
}
