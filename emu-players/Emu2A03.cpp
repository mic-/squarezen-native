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

#include "NativeLogger.h"
#include "Emu2A03.h"

const uint8_t Emu2A03::SQUARE_WAVES[4][8] =
{
	{0,0,0,0, 0,0,0,1},
	{1,0,0,0, 0,0,0,1},
	{1,0,0,0, 0,1,1,1},
	{0,1,1,1, 1,1,1,0}
};


const uint16_t Emu2A03::VOL_TB[] = {
	0,5,10,15,
	20,25,30,35,
	40,45,50,55,
	60,65,70,75
};


const uint8_t Emu2A03::LENGTH_COUNTERS[32] =
{
	10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};


void Emu2A03LengthCounter::Reset()
{
	mStep = 0;
}

void Emu2A03LengthCounter::Step()
{
	if (mStep) {
		mStep--;
	}
}


void Emu2A03LinearCounter::Reset()
{
	mStep = 0;
}

void Emu2A03LinearCounter::Step()
{
	if (mStep) {
		mStep--;
	}
}


void Emu2A03EnvelopeGenerator::Reset()
{
	// TODO: fill out
}

void Emu2A03EnvelopeGenerator::Step()
{
	// TODO: fill out
}


void Emu2A03SweepUnit::Reset()
{
	// TODO: fill out
}

void Emu2A03SweepUnit::Step()
{
	// TODO: fill out
}


void Emu2A03Channel::Reset()
{
	// TODO: fill out
}


void Emu2A03Channel::Step()
{
	// TODO: fill out
}

void Emu2A03Channel::Write(uint32_t addr, uint8_t val)
{
	uint8_t reg = addr & 0x0F;

	switch (mIndex) {
	case Emu2A03::PULSE1:
		break;
	case Emu2A03::PULSE2:
		break;
	case Emu2A03::TRIANGLE:
		if (0x0B == reg) {
			mPeriod = (mPeriod & 0xff) | ((uint16_t)(val & 7) << 8);
			mLinC.mReload = true;
			if (mChip->mStatus & (1 << Emu2A03::TRIANGLE)) {
				mLC.mPos = Emu2A03::LENGTH_COUNTERS[val >> 3] >> 1;
			}
		}
		break;
	case Emu2A03::NOISE:
		break;
	}
}


void Emu2A03::Reset()
{
	for (int i = Emu2A03::PULSE1; i <= Emu2A03::NOISE; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}

	mCycleCount = 0;
	mMaxFrameCount = 3;
}


void Emu2A03::SetClock(uint32_t clockHz, uint32_t fps)
{
	mFrameCycles = clockHz / (fps * 4);
}


void Emu2A03::Step()
{
	if (mCycleCount == 0) {
		if (mCurFrame < 4) {
			if ((mCurFrame & 1) == (mMaxFrameCount & 1)) {
				for (int i = Emu2A03::PULSE1; i <= Emu2A03::NOISE; i++) {
					mChannels[i].mLC.Step();
					if (i <= Emu2A03::PULSE2) {
						mChannels[i].mSU.Step();
					}
				}
			}
			for (int i = Emu2A03::PULSE1; i <= Emu2A03::NOISE; i++) {
				mChannels[i].mEG.Step();
			}
			mChannels[Emu2A03::TRIANGLE].mLinC.Step();
		}
		if (mCurFrame == 3 && mMaxFrameCount == 3 && mGenerateFrameIRQ) {
			// TODO: generate frame IRQ
		}
		mCurFrame++;
		if (mCurFrame > mMaxFrameCount) {
			mCurFrame = 0;
		}
	}

	mCycleCount++;
	if (mCycleCount >= mFrameCycles) {
		mCycleCount = 0;
	}

	for (int i = Emu2A03::PULSE1; i <= Emu2A03::NOISE; i++) {
		mChannels[i].Step();
	}
}


void Emu2A03::Write(uint32_t addr, uint8_t data)
{
	if (addr >= 0x4000 && addr <= 0x4003) {
		mChannels[Emu2A03::PULSE1].Write(addr, data);

	} else if (addr >= 0x4004 && addr <= 0x4007) {
		mChannels[Emu2A03::PULSE2].Write(addr, data);

	} else if (addr >= 0x4008 && addr <= 0x400B) {
		mChannels[Emu2A03::TRIANGLE].Write(addr, data);

	} else if (addr >= 0x400C && addr <= 0x400F) {
		mChannels[Emu2A03::NOISE].Write(addr, data);

	} else if (addr == 0x4017) {
		mGenerateFrameIRQ = ((data & 0x40) == 0);
		mMaxFrameCount = (data & 0x80) ? 4 : 3;
	}
}
