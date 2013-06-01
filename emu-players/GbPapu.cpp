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

#include <FBase.h>
#include "GbPapu.h"
#include "GbZ80.h"


const uint8_t GbPapuChip::SQUARE_WAVES[4][32] =
{
		// 12.5%
		{1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
		// 25%
		{1,1,1,1, 1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
		// 50%
		{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
		// 75%
		{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 0,0,0,0, 0,0,0,0}
};

const uint16_t GbPapuChip::VOL_TB[] = {
		0,5,10,15,
		20,25,30,35,
		40,45,50,55,
		60,65,72,82
		/*	0x0000,0x0000,0x0000,0x0001,
	0x0001,0x0002,0x0003,0x0005,
	0x0007,0x000A,0x000E,0x0014,
	0x001D,0x0029,0x003A,0x0052*/
};


void GbPapuLengthCounter::Reset()
{
	mPos = mStep = 0;
	mPeriod = DMG_CLOCK / 256;
}

void GbPapuLengthCounter::Step()
{
	mPos++;
	if (mPos >= mPeriod) {
		mPos -= mPeriod;
		if (mStep < mMax) {
			mStep++;
		}
	}
}

int GbPapuLengthCounter::GetMask() const
{
	return (mUse && mStep == mMax) ? 0 : -1;
}


void GbPapuEnvelopeGenerator::Reset()
{
	mPos = mStep = 0;
	mPeriod = DMG_CLOCK / 64;
}

void GbPapuEnvelopeGenerator::Step()
{
	if (mMax) mPos++;
	if (mPos >= mPeriod*mMax) {
		mPos -= mPeriod*mMax;
		if (mStep < mMax) {
			mStep++;
			if (-1 == mDirection && mChannel->mCurVol > 0) {
				mChannel->mCurVol--;
			} else if (1 == mDirection && mChannel->mCurVol < 0x0F) {
				mChannel->mCurVol++;
			}
		}
	}
}


void GbPapuChannel::Reset()
{
	mLC.Reset();
	mLC.mUse = false;
	mLC.mMax = 0;

	mEG.Reset();
	mEG.SetChannel(this);
	mEG.mMax = 0;
	mEG.mDirection = 0;

	mWaveStep = 0;
	mPhase = 0;
	mPeriod = 0;
	mDuty = 0;
	mOut = -1;
	mVol = mCurVol = 0;
}


void GbPapuChannel::Step()
{
	mPos++;
	if (mIndex < 2) {
		// Square wave channels
		mLC.Step();
		mEG.Step();
		if (mPos >= (2048 - mPeriod)) {
			mPos = 0;
			if (mWaveStep == 32) {
				mWaveStep = 0;
			}
			mPhase = GbPapuChip::SQUARE_WAVES[mDuty][mWaveStep++];
		}

	} else if (mIndex == 2) {
		// Waveform channel
		mLC.Step();
		if (mPos >= (2048 - mPeriod)*2) {
			mPos = 0;
			if (mWaveStep == 32) {
				mWaveStep = 0;
			}
			mCurVol = (mChip->mWaveformRAM[mWaveStep >> 1] >> ((mWaveStep & 1) * 4)) & 0x0F;
			mPhase = 1;
			mWaveStep++;
			if (0 == mVol) mCurVol = 0;
			else if (2 == mVol) mCurVol >>= 1;
			else if (3 == mVol) mCurVol >>= 2;
		}
	}
}


void GbPapuChannel::Write(uint32_t addr, uint8_t val)
{
	//AppLog("mChannel[%d].Write(%#x, %#x)", mIndex, addr, val);

	if (0xFF11 == addr || 0xFF16 == addr) {
		mDuty = val >> 6;
		mLC.mMax = 64 - (val & 0x3F);

	} else if (0xFF1B == addr) {
		mLC.mMax = 256 - val;

	} else if (0xFF12 == addr || 0xFF17 == addr) {
		mVol = val >> 4;
		mEG.mMax = val & 7;
		mEG.mDirection = -1;
		if (val & 0x08) {
			mCurVol = (mCurVol+1) & 0x0F;
			mEG.mDirection = 1;
		}

	} else if (0xFF1C == addr) {
		mVol = (val >> 5) & 3;

	} else if (0xFF13 == addr || 0xFF18 == addr || 0xFF1D == addr) {
		mPeriod = (mPeriod & 0x700) | val;

	} else if (0xFF14 == addr || 0xFF19 == addr || 0xFF1E == addr) {
		mPeriod = (mPeriod & 0xFF) | (uint16_t)(val & 7) << 8;

		mLC.mUse = ((val & 0x40) == 0x40);

		if (val & 0x80) {
			// Restart
			AppLog("Restarting channel %d (volume %d)", mIndex, mVol);
			mWaveStep = 0;
			mCurVol = mVol;
			mLC.Reset();
			mEG.Reset();
		}
	}
}


void GbPapuChip::Reset()
{
	for (int i = 0; i < 4; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}
}


void GbPapuChip::Step()
{
	for (int i = 0; i < 4; i++) {
		mChannels[i].Step();
	}
}


void GbPapuChip::Write(uint32_t addr, uint8_t val)
{
	//AppLog("GbPapu::Write(%#x, %#x)", addr, val);

	if (addr >= 0xFF10 && addr <= 0xFF14) {
		mChannels[0].Write(addr, val);

	} else if (addr >= 0xFF16 && addr <= 0xFF19) {
		mChannels[1].Write(addr, val);

	} else if (addr >= 0xFF1A && addr <= 0xFF1E) {
		mChannels[2].Write(addr, val);

	} else if (addr >= 0xFF20 && addr <= 0xFF23) {
		mChannels[3].Write(addr, val);

	} else if (addr >= 0xFF30 && addr <= 0xFF3F) {
		mWaveformRAM[addr & 0x0F] = val;
	}
}

