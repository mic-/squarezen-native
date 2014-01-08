/*
 * ARM7TDMI.h
 *
 *  Created on: Oct 29, 2013
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

#ifndef ARM7TDMI_H_
#define ARM7TDMI_H_

#include <stdint.h>
#include "EmuCommon.h"
#include "CpuBase.h"
#include "MemoryMapper.h"

class ARM7TDMI : public CpuBase
{
public:
	virtual void Reset();
	virtual void Run(uint32_t maxCycles);
	//void SetMapper(MemoryMapper *mapper) { mMemory = mapper; }

	enum {
		FLAG_T = 0x20,
		FLAG_F = 0x40,
		FLAG_I = 0x80,
		FLAG_Q = (1<<27),
		FLAG_V = (1<<28),
		FLAG_C = (1<<29),
		FLAG_Z = (1<<30),
		FLAG_N = (1<<31),
	};

	uint32_t mCycles;
	uint32_t mRegs[16];
	uint32_t mRegsUsr[16];
	uint32_t mRegsFiq[16];
	uint32_t mRegsSvc[16];
	uint32_t mRegsAbt[16];
	uint32_t mRegsIrq[16];
	uint32_t mRegsUnd[16];
	uint32_t *mRegBank;

	uint32_t 	mCpsr;
	uint32_t 	mSpsrFiq, mSpsrSvc, mSpsrAbt, mSpsrIrq, mSpsrUnd;
	uint32_t	*mSpsr;

	uint32_t	mFlags[9];
	uint32_t	mFlagsUsr[9];
	uint32_t	mFlagsIrq[9];
	uint32_t	*mCurFlags;

	typedef void (ARM7TDMI::*InstructionDecoder)(uint32_t);
	typedef void (ARM7TDMI::*AluOp)(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags);

	void AluADC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluADD(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluAND(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluASR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluBIC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluCMN(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluCMP(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluEOR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluLSL(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluLSR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluMOV(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluMUL(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluMVN(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluNEG(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluORR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluROR(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluRSB(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluRSC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluSBC(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluSUB(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluTEQ(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);
	void AluTST(uint32_t rd, uint32_t operand1, uint32_t operand2, bool updateFlags = false);

	void ThumbType00(uint32_t instruction);
	void ThumbType01(uint32_t instruction);
	void ThumbType0203(uint32_t instruction);
	void ThumbType04(uint32_t instruction);
	void ThumbType05(uint32_t instruction);
	void ThumbType06(uint32_t instruction);
	void ThumbType07(uint32_t instruction);
	void ThumbType08(uint32_t instruction);
	void ThumbType09(uint32_t instruction);
	void ThumbType0A(uint32_t instruction);
	void ThumbType0B(uint32_t instruction);
	void ThumbType0C(uint32_t instruction);
	void ThumbType0D(uint32_t instruction);
	void ThumbType0E(uint32_t instruction);
	void ThumbType0F(uint32_t instruction);

private:
	MAKE_NON_COPYABLE(ARM7TDMI);

	inline void DecodeARM(uint32_t instruction);
	inline void DecodeThumb(uint32_t instruction);

	AluOp armDataProcOps[16];
	AluOp thumbAluOps[16];
};

#endif	/* ARM7TDMI_H_ */

