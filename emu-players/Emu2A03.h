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
		PULSE1 = 0,
		PULSE2 = 1,
		TRIANGLE = 2,
		NOISE = 3,
		DMC = 4,
	};

	void SetClock(uint32_t clockHz, uint32_t fps);

	static const uint8_t SQUARE_WAVES[4][8];
	static const uint16_t VOL_TB[];
	static const uint8_t LENGTH_COUNTERS[32];

	Emu2A03Channel mChannels[4];
	uint8_t mRegs[0x18];
	uint32_t mCycleCount, mFrameCycles;
	uint8_t mStatus;
	bool mGenerateFrameIRQ;
	uint8_t mMaxFrameCount, mCurFrame;
};


#endif /* EMU2A03_H_ */
