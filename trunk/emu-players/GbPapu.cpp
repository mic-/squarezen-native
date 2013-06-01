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
	41,47,53,59,
	65,71,77,82
	/*0x0000,0x0000,0x0000,0x0001,
	0x0001,0x0002,0x0003,0x0005,
	0x0007,0x000A,0x000E,0x0014,
	0x001D,0x0029,0x003A,0x0052*/
};

const uint16_t GbPapuChip::NOISE_PERIODS[] = {
		8, 16, 32, 48,
		64, 80, 96, 112
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
		mStep++;
		if (-1 == mDirection && mChannel->mCurVol > 0) {
			mChannel->mCurVol--;
		} else if (1 == mDirection && mChannel->mCurVol < 0x0F) {
			mChannel->mCurVol++;
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
	mLfsr = 0x7FFF;
	mLfsrWidth = 15;
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

	} else if (mIndex == 3) {
		mLC.Step();
		mEG.Step();
		if (mPeriod && mPos >= mPeriod) {
			mPos = 0;
			mPhase = (mLfsr & 1) ^ 1;
			mLfsr = (mLfsr >> 1) | (((mLfsr ^ (mLfsr >> 1)) & 1) << 14);
			if (mLfsrWidth == 7) {
				mLfsr &= ~0x40;
				mLfsr |= (mLfsr & 0x4000) >> 8;
			}
		}
	}
}


void GbPapuChannel::Write(uint32_t addr, uint8_t val)
{
	//AppLog("mChannel[%d].Write(%#x, %#x)", mIndex, addr, val);
	int prevDirection;

	switch (addr) {
	case 0xFF11:
	case 0xFF16:
	case 0xFF20:
		mDuty = val >> 6;
		mLC.mMax = 64 - (val & 0x3F);
		break;

	case 0xFF1B:
		mLC.mMax = 256 - val;
		break;

	case 0xFF12:	// NR12
	case 0xFF17:	// NR22
	case 0xFF21:	// NR42
		mVol = val >> 4;
		prevDirection = mEG.mDirection;
		mEG.mDirection = (val & 0x08) ? 1 : -1;
		if ((mEG.mDirection == 1) && (mEG.mMax == 0)) {
			mCurVol = (mCurVol + 1) & 0x0F;
		} else if ((mEG.mDirection == -1) && (mEG.mMax == 0)) {
			mCurVol = (mCurVol + 2) & 0x0F;
		} else if ((mEG.mDirection != prevDirection) && (mEG.mMax != 0)) {
			mCurVol = (16 - mCurVol) & 0x0F;
		}
		if (addr==0xFF21) {
			AppLog("Noise: mVol %d, direction %d, steps %d", mVol, mEG.mDirection, val&7);
		}
		mEG.mMax = val & 7;
		break;

	case 0xFF1C:
		mVol = (val >> 5) & 3;
		break;

	case 0xFF13:
	case 0xFF18:
	case 0xFF1D:
		mPeriod = (mPeriod & 0x700) | val;
		break;

	case 0xFF22:
		mPeriod = GbPapuChip::NOISE_PERIODS[val & 7]; //8 * (val & 7);
		//mPeriod = (mPeriod == 0) ? 4 : mPeriod;
		mPeriod *= ((val >> 4) < 14) ? (1 << ((val >> 4) + 1)) : 0;
		AppLog("Noise period = %d [s %d, r %d] (%d Hz)", mPeriod, val&7, val>>4, (DMG_CLOCK/mPeriod));
		mLfsrWidth = (val & 8) ? 7 : 15;
		break;

	case 0xFF14:	// NR14
	case 0xFF19:	// Nr24
	case 0xFF1E:	// NR34
	case 0xFF23:	// NR44
		if (0xFF23 != addr) {
			mPeriod = (mPeriod & 0xFF) | (uint16_t)(val & 7) << 8;
		}

		mLC.mUse = ((val & 0x40) == 0x40);

		if (val & 0x80) {
			// Restart
			//AppLog("Restarting channel %d (volume %d)", mIndex, mVol);
			mWaveStep = 0;
			mCurVol = mVol;
			mLC.Reset();
			mEG.Reset();
			if (0xFF23 == addr) {
				mLfsr = 0x7FFF;
			}
		}
		break;

	default:
		break;
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

int GbPapuChip::ChannelEnabled(uint8_t index) const
{
	int enabled = mChannels[index].mLC.GetMask();
	enabled = (mNR52 & 0x80) ? enabled : 0;
	return ((mNR51 & (1 << index)) | (mNR51 & (1 << (index+4)))) ? enabled : 0;
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

	} else if (0xFF25 == addr) {
		mNR51 = val;
		AppLog("NR51 = %#x", val);

	} else if (0xFF26 == addr) {
		mNR52 = val;
		AppLog("NR52 = %#x", val);

	} else if (addr >= 0xFF30 && addr <= 0xFF3F) {
		mWaveformRAM[addr & 0x0F] = val;
	}
}

