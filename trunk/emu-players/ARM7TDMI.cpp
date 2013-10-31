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

#define THUMB_GET_RO_RB_RD(instr, Ro, Rb, Rd) \
	Rd = (instr) & 7; \
	Rb = (instr >> 3) & 7; \
	Ro = (instr >> 6) & 7

#define THUMB_GET_RD_IMM8(instr, Rd, imm8) \
	Rd = (instr >> 8) & 7; \
	imm8 = (instr) & 0xFF

#define THUMB_UPDATE_NZ(val32) \
	mCpsr |= val32 & FLAG_N; \
	mCpsr |= val32 ? 0 : FLAG_Z

/* ToDo: finish
#define THUMB_ADD(Rd, op1, op2) \
	mRegs[Rd] = op1 + op2
*/

static const ARM7TDMI::InstructionDecoder ARM_DECODER_TABLE[] =
{

};

static const ARM7TDMI::InstructionDecoder THUMB_DECODER_TABLE[16] =
{
	&ARM7TDMI::ThumbType00,
	&ARM7TDMI::ThumbType01,
	&ARM7TDMI::ThumbType0203,
	&ARM7TDMI::ThumbType0203,
	&ARM7TDMI::ThumbType04,
	&ARM7TDMI::ThumbType05,
	&ARM7TDMI::ThumbType06,
	&ARM7TDMI::ThumbType07,
	&ARM7TDMI::ThumbType08,
	&ARM7TDMI::ThumbType09,
	&ARM7TDMI::ThumbType0A,
	&ARM7TDMI::ThumbType0B,
	&ARM7TDMI::ThumbType0C,
	&ARM7TDMI::ThumbType0D,
	&ARM7TDMI::ThumbType0E,
	&ARM7TDMI::ThumbType0F,
};


void ARM7TDMI::Reset()
{
	// ToDo: implement

	armDataProcOps[0x00] = &ARM7TDMI::AluAND;
	armDataProcOps[0x01] = &ARM7TDMI::AluEOR;
	armDataProcOps[0x02] = &ARM7TDMI::AluSUB;
	armDataProcOps[0x03] = &ARM7TDMI::AluRSB;
	armDataProcOps[0x04] = &ARM7TDMI::AluADD;
	armDataProcOps[0x05] = &ARM7TDMI::AluADC;
	armDataProcOps[0x06] = &ARM7TDMI::AluSBC;
	armDataProcOps[0x07] = &ARM7TDMI::AluRSC;
	armDataProcOps[0x08] = &ARM7TDMI::AluTST;
	armDataProcOps[0x09] = &ARM7TDMI::AluTEQ;
	armDataProcOps[0x0A] = &ARM7TDMI::AluCMP;
	armDataProcOps[0x0B] = &ARM7TDMI::AluCMN;
	armDataProcOps[0x0C] = &ARM7TDMI::AluORR;
	//armDataProcOps[0x0D] = &ARM7TDMI::AluMOV;
	armDataProcOps[0x0E] = &ARM7TDMI::AluBIC;
	armDataProcOps[0x0F] = &ARM7TDMI::AluMVN;

	thumbAluOps[0x00] = &ARM7TDMI::AluAND;
	thumbAluOps[0x01] = &ARM7TDMI::AluEOR;
	thumbAluOps[0x02] = &ARM7TDMI::AluLSL;
	thumbAluOps[0x03] = &ARM7TDMI::AluLSR;
	thumbAluOps[0x04] = &ARM7TDMI::AluASR;
	thumbAluOps[0x05] = &ARM7TDMI::AluADC;
	thumbAluOps[0x06] = &ARM7TDMI::AluSBC;
	thumbAluOps[0x07] = &ARM7TDMI::AluROR;
	thumbAluOps[0x08] = &ARM7TDMI::AluTST;
	thumbAluOps[0x09] = &ARM7TDMI::AluNEG;
	thumbAluOps[0x0A] = &ARM7TDMI::AluCMP;
	thumbAluOps[0x0B] = &ARM7TDMI::AluCMN;
	thumbAluOps[0x0C] = &ARM7TDMI::AluORR;
	thumbAluOps[0x0D] = &ARM7TDMI::AluMUL;
	thumbAluOps[0x0E] = &ARM7TDMI::AluBIC;
	thumbAluOps[0x0F] = &ARM7TDMI::AluMVN;
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
	// d15 d14 d13 d12

	uint32_t decoderBits = (instruction >> 12) & 0xF;
	(this->*THUMB_DECODER_TABLE[decoderBits])(instruction);
}



void ARM7TDMI::AluADC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluADD(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluAND(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluASR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluBIC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluCMN(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluCMP(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluEOR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluLSL(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluLSR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluMOV(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluMUL(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluMVN(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluNEG(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
	AluRSB(rd, operand2, 0, updateFlags);
}

void ARM7TDMI::AluORR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluROR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluRSB(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluRSC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluSBC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluSUB(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluTEQ(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
}

void ARM7TDMI::AluTST(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags)
{
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
	uint32_t rs, rd, shamt;

	THUMB_GET_RS_RD(instruction, rs, rd);
	shamt = (instruction >> 6) & 0x1F;

	if (!(instruction & (1 << 11))) {

		// LSL Rd,Rs,#Offset5

		mCpsr &= ~(FLAG_C | FLAG_Z | FLAG_N);
		mRegs[rd] = mRegs[rs] << shamt;
		mCpsr |= (mRegs[rs] & (1 << (32 - shamt))) ? FLAG_C : 0;
		THUMB_UPDATE_NZ(mRegs[rd]);

	} else {

		// LSR Rd,Rs,#Offset5

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
	}
	mCycles++;
}


void ARM7TDMI::ThumbType01(uint32_t instruction)
{
	uint32_t rs, rd, shamt;

	THUMB_GET_RS_RD(instruction, rs, rd);

	if (!(instruction & (1 << 11))) {

		// ASR Rd,Rs,#Offset5

		shamt = (instruction >> 6) & 0x1F;
		mCpsr &= ~(FLAG_C | FLAG_Z | FLAG_N);
		if (shamt) {
			*(int32_t*)&mRegs[rd] = (int32_t)mRegs[rs] >> shamt;
			mCpsr |= (mRegs[rs] & (1 << (shamt - 1))) ? FLAG_C : 0;
			THUMB_UPDATE_NZ(mRegs[rd]);
		} else {
			*(int32_t*)&mRegs[rd] = (int32_t)mRegs[rs] >> 31;
			mCpsr |= (mRegs[rd]) ? FLAG_C : 0;
			THUMB_UPDATE_NZ(mRegs[rd]);
		}

	} else {
		uint32_t rnImm3 = (instruction >> 6) & 7;
		switch ((instruction >> 9) & 3) {
		case 0:
			// ADD Rd,Rs,Rn
			AluADD(rd, mRegs[rs], mRegs[rnImm3], true);
			break;
		case 1:
			// SUB Rd,Rs,Rn
			AluSUB(rd, mRegs[rs], mRegs[rnImm3], true);
			break;
		case 2:
			// ADD Rd,Rs,#imm3
			AluADD(rd, mRegs[rs], rnImm3, true);
			break;
		case 3:
			// SUB Rd,Rs,#imm3
			AluSUB(rd, mRegs[rs], rnImm3, true);
			break;
		}
	}
	mCycles++;
}


void ARM7TDMI::ThumbType0203(uint32_t instruction)
{
	uint32_t rd, imm;

	THUMB_GET_RD_IMM8(instruction, rd, imm);

	switch ((instruction >> 11) & 3) {
	case 0:
		// MOV Rd,#Offset8
		//AluMOV(rd, mRegs[rd], imm, true);
		break;
	case 1:
		// CMP Rd,#Offset8
		AluCMP(rd, mRegs[rd], imm, true);
		break;
	case 2:
		// ADD Rd,#Offset8
		AluADD(rd, mRegs[rd], imm, true);
		break;
	case 3:
		// SUB Rd,#Offset8
		AluSUB(rd, mRegs[rd], imm, true);
		break;
	}
}


void ARM7TDMI::ThumbType04(uint32_t instruction)
{
	uint32_t rd, rs, imm;

	if (instruction & (1 << 11)) {

		// LDR Rd,[PC, #WordOffset8]
		THUMB_GET_RD_IMM8(instruction, rd, imm);
		uint32_t addr = (mRegs[15] & 0xFFFFFFFD) + (imm << 2);
		mRegs[rd] = mMemory->ReadWord(addr);

	} else if (instruction & (1 << 10)) {

		// Hi register ops
		THUMB_GET_RS_RD(instruction, rs, rd);
		uint32_t h1 = (instruction >> 4) & 8;
		uint32_t h2 = (instruction >> 3) & 8;

		switch ((instruction >> 8) & 3) {
		case 0:
			// ADD Rd, Rs
			AluADD(rd + h1, mRegs[rd + h1], mRegs[rs + h2]);
			break;
		case 1:
			// CMP Rd, Rs
			AluCMP(rd + h1, mRegs[rd + h1], mRegs[rs + h2], true);
			break;
		case 2:
			// MOV Rd, Rs
			mRegs[rd + h1] = mRegs[rs + h2];
			break;
		case 3:
			// BX Rs
			// ToDo: implement
			break;
		}

	} else {
		// ALU operations
		uint32_t rs, rd;
		THUMB_GET_RS_RD(instruction, rs, rd);
		(this->*thumbAluOps[(instruction >> 6) & 0x0F])(rd, mRegs[rd], mRegs[rs], true);
	}
}

void ARM7TDMI::ThumbType05(uint32_t instruction)
{
	uint32_t rd, rb, ro;

	THUMB_GET_RO_RB_RD(instruction, ro, rb, rd);

	switch ((instruction >> 9) & 7) {
	case 0:
		// STR Rd,[Rb,Ro]
		mMemory->WriteWord(mRegs[rb] + mRegs[ro], mRegs[rd]);
		// ToDo: add cycles
		break;
	case 1:
		// STRH Rd,[Rb,Ro]
		mMemory->WriteHalfWord(mRegs[rb] + mRegs[ro], mRegs[rd]);
		// ToDo: add cycles
		break;
	case 2:
		// STRB Rd,[Rb,Ro]
		mMemory->WriteByte(mRegs[rb] + mRegs[ro], mRegs[rd]);
		// ToDo: add cycles
		break;
	case 3:
		// LDSB Rd,[Rb,Ro]
		break;
	case 4:
		// LDR Rd,[Rb,Ro]
		mRegs[rd] = mMemory->ReadWord(mRegs[rb] + mRegs[ro]);
		// ToDo: add cycles
		break;
	case 5:
		// LDRH Rd,[Rb,Ro]
		mRegs[rd] = mMemory->ReadHalfWord(mRegs[rb] + mRegs[ro]);
		// ToDo: add cycles
		break;
	case 6:
		// LDRB Rd,[Rb,Ro]
		mRegs[rd] = mMemory->ReadByte(mRegs[rb] + mRegs[ro]);
		// ToDo: add cycles
		break;
	case 7:
		// LDSH Rd,[Rb,Ro]
		break;
	}
}

void ARM7TDMI::ThumbType06(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType07(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType08(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType09(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType0A(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType0B(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType0C(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType0D(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType0E(uint32_t instruction)
{
	// ToDo: implement
}

void ARM7TDMI::ThumbType0F(uint32_t instruction)
{
	// ToDo: implement
}

