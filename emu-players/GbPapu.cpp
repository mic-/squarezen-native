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
#include "GbMemory.h"
#include "GbPapu.h"
#include "GbZ80.h"


const uint8_t GbPapuChip::SQUARE_WAVES[4][8] =
{
/*
	// 12.5%
	{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},
	// 25%
	{1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},
	// 50%
	{1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1, 1,1,1,1, 1,1,1,1},
	// 75%
	{0,0,0,0, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 0,0,0,0}
	*/
/*
	{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,1},
	// 25%
	{1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,1},
	// 50%
	{1,1,0,0, 0,0,0,0, 0,0,1,1, 1,1,1,1},
	// 75%
	{0,0,1,1, 1,1,1,1, 1,1,1,1, 1,1,0,0}*/

		{0,0,0,0, 0,0,0,1},
		{1,0,0,0, 0,0,0,1},
		{1,0,0,0, 0,1,1,1},
		{0,1,1,1, 1,1,1,0}
};

const uint16_t GbPapuChip::VOL_TB[] = {
	0,5,10,15,
	20,25,30,35,
	41,47,53,59,
	65,71,77,82
};


const uint8_t INITIAL_WAVEFORM[] = {
	0x84, 0x40, 0x43, 0xAA,
	0x2D, 0x78, 0x92, 0x3C,
	0x60, 0x59, 0x59, 0xB0,
	0x34, 0xB8, 0x2E, 0xDA
};


void GbPapuLengthCounter::Reset()
{
	mPos = mStep = 0;
	mPeriod = GBPAPU_EMULATION_CLOCK / 256;
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
	mPeriod = GBPAPU_EMULATION_CLOCK / 64;
}

void GbPapuEnvelopeGenerator::Step()
{
	mPos++;
	if (mMax && mPos >= mPeriod*mMax) {
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
	mOutL = mOutR = -1;
	mVol = mCurVol = 0;
	mLfsr = 0x7FFF;
	mLfsrWidth = 15;
}


void GbPapuChannel::Step()
{
	mPos++;
	if (mIndex <= GbPapuChip::PULSE2) {
		// Square wave channels
		mLC.Step();
		mEG.Step();
		if (mPos >= (2048 - mPeriod)) {
			mPos = 0;
			if (mWaveStep == 8) {
				mWaveStep = 0;
			}
			mPhase = GbPapuChip::SQUARE_WAVES[mDuty][mWaveStep++];
		}

	} else if (mIndex == GbPapuChip::WAVE) {
		// Waveform channel
		mPos++;
		mLC.Step();
		if (mPos >= (2048 - mPeriod)) {
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

	} else if (mIndex == GbPapuChip::NOISE) {
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
	int prevDirection;

	switch (addr) {
	case 0xFF11:	// NR11
	case 0xFF16:	// NR21
	case 0xFF20:	// NR41
		mDuty = val >> 6;
		mLC.mMax = 64 - (val & 0x3F);
		break;

	case 0xFF1B:	// NR31
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
		mEG.mMax = val & 7;
		break;

	case 0xFF1C:	// NR32
		mVol = (val >> 5) & 3;
		NLOGD("GbPapu", "Waveform channel volume %d", mVol);
		break;

	case 0xFF13:	// NR13
	case 0xFF18:	// NR23
	case 0xFF1D:	// NR33
		mPeriod = (mPeriod & 0x700) | val;
		break;

	case 0xFF22:	// NR43
		mPeriod = 4 * (val & 7);
		mPeriod = (mPeriod == 0) ? 2 : mPeriod;
		mPeriod = ((val >> 4) < 14) ? (mPeriod << ((val >> 4) + 0)) : 0;
		mLfsrWidth = (val & 8) ? 7 : 15;
		NLOGD("GbPapu", "Noise period = %d [s %d, r %d, w %d] (%d Hz)", mPeriod, val&7, val>>4, mLfsrWidth, (GBPAPU_EMULATION_CLOCK/mPeriod));
		break;

	case 0xFF14:	// NR14
	case 0xFF19:	// NR24
	case 0xFF1E:	// NR34
	case 0xFF23:	// NR44
		if (0xFF23 != addr) {
			mPeriod = (mPeriod & 0xFF) | (uint16_t)(val & 7) << 8;
		}

		mLC.mUse = ((val & 0x40) == 0x40);

		if (val & 0x80) {
			// Restart
			//NativeLog("Restarting channel %d (volume %d)", mIndex, mVol);
			//mWaveStep = 0;
			mPos = 0;
			mCurVol = mVol;
			mLC.Reset();
			mEG.Reset();
			if (mIndex == GbPapuChip::WAVE) {
				NLOGD("GbPapu", "Restarting channel %d with period %d, duty %d, volume %d, EG dir %d, EG max %d", mIndex, 2048-mPeriod, mDuty, mVol, mEG.mDirection, mEG.mMax);
			}				
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
	for (int i = GbPapuChip::PULSE1; i <= GbPapuChip::NOISE; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}

	mem_write_8(0xFF10, 0x80);	// NR10
	mem_write_8(0xFF11, 0xBF);	// NR11
	mem_write_8(0xFF10, 0x80); 	// NR10
	mem_write_8(0xFF11, 0xBF); 	// NR11
	mem_write_8(0xFF12, 0xF3); 	// NR12
	mem_write_8(0xFF14, 0xBF); 	// NR14
	mem_write_8(0xFF16, 0x3F); 	// NR21
	mem_write_8(0xFF17, 0x00); 	// NR22
	mem_write_8(0xFF19, 0xBF); 	// NR24
	mem_write_8(0xFF1A, 0x7F); 	// NR30
	mem_write_8(0xFF1B, 0xFF); 	// NR31
	mem_write_8(0xFF1C, 0x9F); 	// NR32
	mem_write_8(0xFF1E, 0xBF); 	// NR33
	mem_write_8(0xFF20, 0xFF); 	// NR41
	mem_write_8(0xFF21, 0x00); 	// NR42
	mem_write_8(0xFF22, 0x00); 	// NR43
	mem_write_8(0xFF23, 0xBF); 	// NR30
	mem_write_8(0xFF24, 0x77); 	// NR50
    mem_write_8(0xFF25, 0xF3); 	// NR51
	mem_write_8(0xFF26, 0xF1);	// NR52

	for (int i = 0; i < 0x10; i++) {
		mem_write_8(0xFF30 + i, INITIAL_WAVEFORM[i]);
	}
}


void GbPapuChip::Step()
{
	for (int i = GbPapuChip::PULSE1; i <= GbPapuChip::NOISE; i++) {
		mChannels[i].Step();
	}
}

int GbPapuChip::ChannelEnabledLeft(uint8_t index) const
{
	int enabled = mChannels[index].mLC.GetMask();
	enabled = (mNR52 & 0x80) ? enabled : 0;
	return (mNR51 & (1 << (index + 4))) ? enabled : 0;
}

int GbPapuChip::ChannelEnabledRight(uint8_t index) const
{
	int enabled = mChannels[index].mLC.GetMask();
	enabled = (mNR52 & 0x80) ? enabled : 0;
	return (mNR51 & (1 << index)) ? enabled : 0;
}

void GbPapuChip::Write(uint32_t addr, uint8_t val)
{
	//NativeLog("GbPapu::Write(%#x, %#x)", addr, val);

	if (addr >= 0xFF10 && addr <= 0xFF14) {
		mChannels[GbPapuChip::PULSE1].Write(addr, val);

	} else if (addr >= 0xFF16 && addr <= 0xFF19) {
		mChannels[GbPapuChip::PULSE2].Write(addr, val);

	} else if (addr >= 0xFF1A && addr <= 0xFF1E) {
		NLOGD("GbPapu", "Writing to waveform channel: %#x, %#x", addr, val);
		mChannels[GbPapuChip::WAVE].Write(addr, val);

	} else if (addr >= 0xFF20 && addr <= 0xFF23) {
		mChannels[GbPapuChip::NOISE].Write(addr, val);

	} else if (0xFF25 == addr) {
		mNR51 = val;
		NLOGD("GbPapu", "NR51 = %#x", val);

	} else if (0xFF26 == addr) {
		mNR52 = val;
		NLOGD("GbPapu", "NR52 = %#x", val);

	} else if (addr >= 0xFF30 && addr <= 0xFF3F) {
		NLOGD("GbPapu", "Writing to waveform RAM %#x, %#x", addr, val);
		mWaveformRAM[addr & 0x0F] = val;
	}
}

