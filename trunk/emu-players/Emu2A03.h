/*
 * Emu2A03.h
 *
 *  Created on: Jun 2, 2013
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

#ifndef EMU2A03_H_
#define EMU2A03_H_

#include <stdint.h>
#include "Oscillator.h"

class Emu2A03;
class Emu2A03Channel;


class Emu2A03LengthCounter : public Oscillator
{
public:
	virtual ~Emu2A03LengthCounter() {}

	virtual void Reset();
	virtual void Step();

	void SetChannel(Emu2A03Channel *channel) { mChannel = channel; }

	int GetMask() const;

	Emu2A03Channel *mChannel;
	uint32_t mMax;
	bool mUse;
};

class Emu2A03LinearCounter : public Oscillator
{
public:
	virtual ~Emu2A03LinearCounter() {}

	virtual void Reset();
	virtual void Step();

	void SetChannel(Emu2A03Channel *channel) { mChannel = channel; }

	Emu2A03Channel *mChannel;
	uint32_t mMax;
	bool mUse;
	bool mReload;
};

class Emu2A03EnvelopeGenerator : public Oscillator
{
public:
	virtual ~Emu2A03EnvelopeGenerator() {}

	virtual void Reset();
	virtual void Step();

	void SetChannel(Emu2A03Channel *channel) { mChannel = channel; }

	Emu2A03Channel *mChannel;
	uint32_t mMax;
	int16_t mDirection;
	uint8_t mOut;
	bool mUse;
};


class Emu2A03SweepUnit : public Oscillator
{
public:
	virtual ~Emu2A03SweepUnit() {}

	virtual void Reset();
	virtual void Step();

	void SetChannel(Emu2A03Channel *channel) { mChannel = channel; }

	Emu2A03Channel *mChannel;
};


class Emu2A03Channel : public Oscillator
{
public:
	virtual ~Emu2A03Channel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	void SetChip(Emu2A03 *chip) { mChip = chip; }
	void SetIndex(uint8_t index) { mIndex = index; }

	Emu2A03LengthCounter mLC;
	Emu2A03LinearCounter mLinC;
	Emu2A03EnvelopeGenerator mEG;
	Emu2A03SweepUnit mSU;
	Emu2A03 *mChip;
	int16_t mOut;
	uint8_t mWaveStep;
	uint16_t mOutputMask;
	uint16_t mLfsr, mLfsrWidth;
	uint16_t mIndex;
	uint16_t mVol, mCurVol;
	uint8_t mPhase;
	uint8_t mDuty;
};


class Emu2A03
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	enum
	{
		CHN_PULSE1 = 0,
		CHN_PULSE2 = 1,
		CHN_TRIANGLE = 2,
		CHN_NOISE = 3,
		CHN_DMC = 4,
	};

	enum
	{
		R_PULSE1_DUTY_ENVE = 0x00,
		R_PULSE1_SWEEP = 0x01,
		R_PULSE1_PERLO = 0x02,
		R_PULSE1_PERHI_LEN = 0x03,
		R_PULSE2_DUTY_ENVE = 0x04,
		R_PULSE2_SWEEP = 0x05,
		R_PULSE2_PERLO = 0x06,
		R_PULSE2_PERHI_LEN = 0x07,
		R_TRIANGLE_LIN = 0x08,
		R_TRIANGLE_PERLO = 0x0A,
		R_TRIANGLE_PERHI_LEN = 0x0B,
		R_NOISE_ENVE = 0x0C,
		R_NOISE_MODE_PER = 0x0E,
		R_NOISE_LEN = 0x0F,
		R_STATUS = 0x15,
	};

	void SetClock(uint32_t clockHz, uint32_t fps);

	static const uint8_t SQUARE_WAVES[4][8];
	static const uint8_t TRIANGLE_WAVE[32];
	static const uint16_t VOL_TB[];
	static const uint8_t LENGTH_COUNTERS[32];
	static const uint16_t NOISE_PERIODS[2][16];

	Emu2A03Channel mChannels[4];
	uint8_t mRegs[0x18];
	uint32_t mCycleCount, mFrameCycles;
	uint8_t mStatus;
	bool mGenerateFrameIRQ;
	uint8_t mMaxFrameCount, mCurFrame;
};


#endif /* EMU2A03_H_ */
