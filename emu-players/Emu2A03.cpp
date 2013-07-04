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

//#define NLOG_LEVEL_VERBOSE 0

#include "NativeLogger.h"
#include "Emu2A03.h"

const uint8_t Emu2A03::SQUARE_WAVES[4][8] =
{
	{0,1,0,0,0,0,0,0},
	{0,1,1,0,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{1,0,0,1,1,1,1,1}
};

const uint8_t Emu2A03::TRIANGLE_WAVE[32] =
{
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4,3,2,1,0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15
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

const uint16_t Emu2A03::NOISE_PERIODS[2][16] =
{
	{4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068},	// NTSC
	{4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778}		// PAL
};

void Emu2A03LengthCounter::Reset()
{
	mStep = 0;
}

void Emu2A03LengthCounter::Step()
{
	if (mChannel->mChip->mRegs[Emu2A03::R_STATUS] & (1 << mChannel->mIndex)) {
		uint8_t reg = mChannel->mChip->mRegs[Emu2A03::R_PULSE1_DUTY_ENVE + mChannel->mIndex * 4];
		if (!(reg & ((mChannel->mIndex == Emu2A03::CHN_TRIANGLE) ? 0x80 : 0x20))) {
			if (mStep) {
				mStep--;
			}
			if (mChannel->mIndex != Emu2A03::CHN_TRIANGLE) {
				// This is handled in Emu2A03LinearCounter::Step for the triangle channel
				mChannel->mOutputMask = mStep ? 0xFFFF : 0;
			}
		}
	}
}


void Emu2A03LinearCounter::Reset()
{
	mStep = 0;
}

void Emu2A03LinearCounter::Step()
{
	if (mReload) {
		mStep = mChannel->mChip->mRegs[Emu2A03::R_TRIANGLE_LIN] & 0x7F;
	} else if (mStep) {
		mStep--;
	}
	if (!(mChannel->mChip->mRegs[Emu2A03::R_TRIANGLE_LIN] & 0x80)) {
		mReload = false;
	}
	mChannel->mOutputMask = (mStep && mChannel->mLC.mStep) ? 0xFFFF : 0;
}


void Emu2A03EnvelopeGenerator::Reset()
{
	if (mChannel->mIndex != Emu2A03::CHN_TRIANGLE) {
		mStep = (mChannel->mChip->mRegs[Emu2A03::R_PULSE1_DUTY_ENVE + mChannel->mIndex * 4] & 0x0F) + 1;
		mOut = 0x0F;
	}
}

void Emu2A03EnvelopeGenerator::Step()
{
	if (mChannel->mIndex != Emu2A03::CHN_TRIANGLE) {
		uint8_t enve = mChannel->mChip->mRegs[Emu2A03::R_PULSE1_DUTY_ENVE + mChannel->mIndex * 4];
		if (!(enve & 0x10)) {
			mStep--;
			if (!mStep) {
				mStep = (enve = 0x0F) + 1;
				if (!mOut) {
					if (enve & 0x20) {
						// Looping envelope
						mOut = 0x0F;
					}
				}
				mChannel->mVol = mOut;
				mOut--;
			}
		}
	}
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
	mLC.SetChannel(this);
	mLC.Reset();
	mLC.mUse = false;
	mLC.mMax = 0;

	mEG.SetChannel(this);
	mEG.Reset();
	mEG.mMax = 0;
	mEG.mDirection = 0;

	mLinC.SetChannel(this);
	mLinC.Reset();

	mWaveStep = 0;
	mPhase = 0;
	mPeriod = 0;
	mDuty = 0;
	mOut = -1;
	mPos = 0;
	mVol = mCurVol = 0;
	mLfsr = 0x0001;
	//mLfsrWidth = 15;
}


void Emu2A03Channel::Step()
{
	mPos++;
	if (mIndex <= Emu2A03::CHN_PULSE2) {
		if (mPos >= mPeriod*2) {
			mPos = 0;
			if (mWaveStep == 8) {
				mWaveStep = 0;
			}
			mPhase = Emu2A03::SQUARE_WAVES[mDuty][mWaveStep++];
			if (mPeriod < 8) mPhase = 0;
		}

	} else if (mIndex == Emu2A03::CHN_TRIANGLE) {
		if (mPos >= mPeriod + 1) {
			mPos = 0;
			if (mWaveStep == 32) {
				mWaveStep = 0;
			}
			mPhase = 1;
			mVol = Emu2A03::TRIANGLE_WAVE[mWaveStep++];
		}

	} else if (mIndex == Emu2A03::CHN_NOISE) {
		if (mPeriod && mPos >= mPeriod) {
			mPos = 0;
			mPhase = (mLfsr & 1) ^ 1;
			if (mChip->mRegs[Emu2A03::R_NOISE_MODE_PER] & 0x80) {
				mLfsr = (mLfsr >> 1) | (((mLfsr ^ (mLfsr >> 6)) & 1) << 14);
			} else {
				mLfsr = (mLfsr >> 1) | (((mLfsr ^ (mLfsr >> 1)) & 1) << 14);
			}
		}
	}
}

void Emu2A03Channel::Write(uint32_t addr, uint8_t val)
{
	uint8_t reg = addr & 0x0F;

	switch (reg) {
	case Emu2A03::R_PULSE1_DUTY_ENVE:
	case Emu2A03::R_PULSE2_DUTY_ENVE:
	case Emu2A03::R_NOISE_ENVE:
		if (val & 0x10) {
			mVol = val & 0x0F;
		}
		if (reg == Emu2A03::R_NOISE_ENVE) {
			// Noise channel
		} else {
			// Pulse channels
			mDuty = val >> 6;
		}
		break;

	case Emu2A03::R_PULSE1_PERLO:
	case Emu2A03::R_PULSE2_PERLO:
	case Emu2A03::R_TRIANGLE_PERLO:
		mPeriod = (mPeriod & 0x700) | val;
		break;

	case Emu2A03::R_NOISE_MODE_PER:
		mPeriod = Emu2A03::NOISE_PERIODS[0][val & 0x0F];
		break;

	case Emu2A03::R_PULSE1_PERHI_LEN:
	case Emu2A03::R_PULSE2_PERHI_LEN:
	case Emu2A03::R_TRIANGLE_PERHI_LEN:
	case Emu2A03::R_NOISE_LEN:
		if (Emu2A03::R_NOISE_LEN != reg) {
			mPeriod = (mPeriod & 0xff) | ((uint16_t)(val & 7) << 8);
		}
		if (Emu2A03::R_TRIANGLE_PERHI_LEN == reg) {
			mLinC.mReload = true;
		} else {
			mEG.Reset();
			mPos = 0;
			//mWaveStep = 0;
		}
		if (mChip->mRegs[Emu2A03::R_STATUS] & (1 << mIndex)) {
			mLC.mStep = Emu2A03::LENGTH_COUNTERS[val >> 3];
			mOutputMask = mLC.mStep ? 0xFFFF : 0;
		}
		break;

	}
}


void Emu2A03::Reset()
{
	for (int i = Emu2A03::CHN_PULSE1; i <= Emu2A03::CHN_NOISE; i++) {
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
				for (int i = Emu2A03::CHN_PULSE1; i <= Emu2A03::CHN_NOISE; i++) {
					mChannels[i].mLC.Step();
					if (i <= Emu2A03::CHN_PULSE2) {
						mChannels[i].mSU.Step();
					}
				}
			}
			for (int i = Emu2A03::CHN_PULSE1; i <= Emu2A03::CHN_NOISE; i++) {
				mChannels[i].mEG.Step();
			}
			mChannels[Emu2A03::CHN_TRIANGLE].mLinC.Step();
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

	for (int i = Emu2A03::CHN_PULSE1; i <= Emu2A03::CHN_NOISE; i++) {
		mChannels[i].Step();
	}
}


void Emu2A03::Write(uint32_t addr, uint8_t data)
{
	NLOGD("Emu2A03", "Write(%#x, %#x)", addr, data);

	if (addr >= 0x4000 && addr <= 0x4017) {
		mRegs[addr - 0x4000] = data;
	}

	uint8_t reg = addr & 0x1F;

	if (reg >= Emu2A03::R_PULSE1_DUTY_ENVE && reg <= Emu2A03::R_PULSE1_PERHI_LEN) {
		mChannels[Emu2A03::CHN_PULSE1].Write(addr, data);

	} else if (reg >= Emu2A03::R_PULSE2_DUTY_ENVE && reg <= Emu2A03::R_PULSE2_PERHI_LEN) {
		mChannels[Emu2A03::CHN_PULSE2].Write(addr, data);

	} else if (addr >= 0x4008 && addr <= 0x400B) {
		mChannels[Emu2A03::CHN_TRIANGLE].Write(addr, data);

	} else if (addr >= 0x400C && addr <= 0x400F) {
		mChannels[Emu2A03::CHN_NOISE].Write(addr, data);

	} else if (addr == 0x4017) {
		mGenerateFrameIRQ = ((data & 0x40) == 0);
		mMaxFrameCount = (data & 0x80) ? 4 : 3;
	}
}
