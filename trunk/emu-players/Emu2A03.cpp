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

const uint16_t Emu2A03::VOL_TB[] = {
	0,5,10,15,
	20,25,30,35,
	40,45,50,55,
	60,65,70,75
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
	// TODO: fill out
}


void Emu2A03::Reset()
{
	// TODO: fill out
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
				for (int i = 0; i < 4; i++) {
					mChannels[i].mLC.Step();
					if (i < 2) {
						mChannels[i].mSU.Step();
					}
				}
			}
			for (int i = 0; i < 4; i++) {
				mChannels[i].mEG.Step();
			}
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

	for (int i = 0; i < 4; i++) {
		mChannels[i].Step();
	}
}


void Emu2A03::Write(uint32_t addr, uint8_t data)
{
	if (addr >= 0x4000 && addr <= 0x4003) {
		mChannels[0].Write(addr, data);

	} else if (addr >= 0x4004 && addr <= 0x4007) {
		mChannels[1].Write(addr, data);

	} else if (addr >= 0x4008 && addr <= 0x400B) {
		mChannels[2].Write(addr, data);

	} else if (addr >= 0x400C && addr <= 0x400F) {
		mChannels[3].Write(addr, data);

	} else if (addr == 0x4017) {
		mGenerateFrameIRQ = ((data & 0x40) == 0);
		mMaxFrameCount = (data & 0x80) ? 4 : 3;
	}
}
