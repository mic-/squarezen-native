/*
 * MMC5.h
 *
 *  Created on: Apr 14, 2014
 *
 * Copyright 2014 Mic
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

#pragma once

#include <memory>
#include <stdint.h>
#include "EmuCommon.h"
#include "Oscillator.h"

class MMC5;


class MMC5Channel : public Oscillator
{
public:
	MMC5Channel() {}
	virtual ~MMC5Channel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	void SetChip(std::shared_ptr<MMC5> chip) { mChip = chip; }
	void SetIndex(uint8_t index) { mIndex = index; }

	//Emu2A03LengthCounter mLC;
	//Emu2A03LinearCounter mLinC;
	//Emu2A03EnvelopeGenerator mEG;
	std::shared_ptr<MMC5> mChip;
	int16_t mOut;
	int16_t mDacOut;
	uint16_t mWaveStep;
	uint16_t mOutputMask;
	uint16_t mSampleAddr, mSampleLen;
	uint8_t mSample, mSampleBits;
	//uint16_t mLfsr, mLfsrWidth;
	uint16_t mIndex;
	uint16_t mVol, mCurVol;
	uint8_t mPhase;
	uint8_t mDuty;

private:
	MAKE_NON_COPYABLE(MMC5Channel);
};


class MMC5
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	// Channel enumerators
	enum
	{
		CHN_PULSE1   = 0,
		CHN_PULSE2   = 1,
	};

	// Register enumerators
	enum
	{
		R_PULSE1_DUTY_ENVE = 0x00,
		R_PULSE1_SWEEP     = 0x01,
		R_PULSE1_PERLO     = 0x02,
		R_PULSE1_PERHI_LEN = 0x03,
		R_PULSE2_DUTY_ENVE = 0x04,
		R_PULSE2_SWEEP     = 0x05,
		R_PULSE2_PERLO     = 0x06,
		R_PULSE2_PERHI_LEN = 0x07,
	};

	MMC5Channel mChannels[2];
};