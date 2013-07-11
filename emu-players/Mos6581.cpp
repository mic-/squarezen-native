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

#define NLOG_LEVEL_DEBUG 0

#include "NativeLogger.h"
#include "Mos6581.h"

static const uint32_t EG_PERIODS[] = {9, 32, 63, 95, 149, 220, 267, 313, 392, 977, 1953, 3125, 3906, 11718, 19531, 31250};

static uint8_t prevRegValue;
static uint32_t curCycle = 0;


void Mos6581EnvelopeGenerator::Reset()
{
	mPhase = ATTACK;
	mClocked = false;
	mOut = 0;
	mClockDivider = 1;
	mPos = 0;
}

void Mos6581EnvelopeGenerator::Step()
{
	if (mClocked) {
		mPos++;
		if (mPos >= mPeriod*mClockDivider) {
			mPos = 0;
			switch (mPhase) {
			case ATTACK:
				mOut++;
				if (mOut >= 0xFF) {
					mOut = 0xFF;
					if (mChannel->mIndex==2) NLOGV("Mos6581", "Channel %d starting decay phase (time %d)", mChannel->mIndex, curCycle);
					mPhase = DECAY;
					mPeriod = EG_PERIODS[mChannel->mChip->mRegs[Mos6581::R_VOICE1_AD + mChannel->mIndex * 7] & 0x0F];
					mClockDivider = 1;
				}
				break;

			case DECAY:
				if (mOut <= mSustainLevel) {
					mOut = mSustainLevel;
					mClocked = false;
					mPhase = SUSTAIN;
					if (mChannel->mIndex==2) NLOGV("Mos6581", "Channel %d starting sustain phase at level %d (time %d)", mChannel->mIndex, mOut, curCycle);
				} else {
					if (mOut) mOut--;

					if (mOut == 93) mClockDivider = 2;
					else if (mOut == 54) mClockDivider = 4;
					else if (mOut == 26) mClockDivider = 8;
					else if (mOut == 14) mClockDivider = 16;
					else if (mOut == 6) mClockDivider = 30;
					else if (mOut == 0) mClockDivider = 1;
				}
				break;

			/*case SUSTAIN:
				mPhase = RELEASE;
				break;*/

			case RELEASE:
				if (mOut) {
					mOut--;
					if (!mOut) {
						NLOGV("Mos6581", "Channel %d reached zero envelope", mChannel->mIndex);
						mClocked = false;
					}
				}
				if (mOut == 93) { mClockDivider = 2; NLOGV("Mos6581", "Clock divider changing to 2 on channel %d (time %d)", mChannel->mIndex, curCycle); }
				else if (mOut == 54) mClockDivider = 4;
				else if (mOut == 26) mClockDivider = 8;
				else if (mOut == 14) mClockDivider = 16;
				else if (mOut == 6) mClockDivider = 30;
				else if (mOut == 0) mClockDivider = 1;
				break;

			default:
				break;
			}
		}
	}
}


void Mos6581Channel::Reset()
{
	NLOGD("Mos6581Channel", "Reset");

	mEG.SetChannel(this);
	mEG.Reset();

	mPeriod = 0;
	mStep = 0;
	mDuty = 0;
	mOut = -1;
	mLfsr = 0x7FFFF8;
	mOutputMask = 0xFFFF;
}


void Mos6581Channel::Step()
{
	uint8_t ctrl = mChip->mRegs[Mos6581::R_VOICE1_CTRL + mIndex * 7];
	uint16_t triOut = 0, sawOut = 0, pulseOut = 0, noiseOut = 0;
	uint8_t prevChn = (mIndex == 0) ? 2 : mIndex - 1;
	uint8_t nextChn = (mIndex == 2) ? 0 : mIndex + 1;
	uint8_t nextCtrl = mChip->mRegs[Mos6581::R_VOICE1_CTRL + nextChn * 7];

	uint32_t oldPos = mPos;
	if (!(ctrl & Mos6581::VOICE_CTRL_TEST)) {
		mPos = (mPos + mStep) & 0xFFFFFF;
	}

	// Hard sync
	if (nextCtrl & Mos6581::VOICE_CTRL_SYNC) {
		if ((mPos ^ oldPos) & mPos & 0x800000) {
			mChip->mChannels[nextChn].mPos = 0;
		}
	}

	if ((mPos >> 12) >= (0xFFF - mDuty)) {
		pulseOut = 0xFFF;
	}

	triOut = (mPos >> 11) & 0xFFE;
	if (ctrl & Mos6581::VOICE_CTRL_RMOD) {
		// Ring modulation
		if (mChip->mChannels[prevChn].mPos & 0x800000) {
			triOut ^= 0xFFE;
		}
	} else if (mPos & 0x800000) {
		triOut ^= 0xFFE;
	}

	sawOut = mPos >> 12;

	if ((mPos ^ oldPos) & 0xFFF000) {
		mLfsr = (mLfsr << 1) | (((mLfsr >> 22) ^ (mLfsr >> 17)) & 1);
		mLfsr &= 0x7FFFFF;
		//noiseOut = mLfsr >> 11;
		noiseOut = ((mLfsr >> 9) & 0x800) |
				   ((mLfsr >> 8) & 0x400) |
				   ((mLfsr >> 5) & 0x200) |
				   ((mLfsr >> 3) & 0x100) |
				   ((mLfsr >> 2) & 0x080) |
				   ((mLfsr << 1) & 0x040) |
				   ((mLfsr << 3) & 0x020) |
				   ((mLfsr << 4) & 0x010);
	}

	mEG.Step();

	uint32_t out = 0;
	if (ctrl & Mos6581::VOICE_CTRL_TRIANGLE) {
		out = triOut;
		if (ctrl & Mos6581::VOICE_CTRL_SAW) {
			out &= sawOut;
		}
		if (ctrl & Mos6581::VOICE_CTRL_PULSE) {
			out &= pulseOut;
		}
	} else if (ctrl & Mos6581::VOICE_CTRL_SAW) {
		out = sawOut;
		if (ctrl & Mos6581::VOICE_CTRL_PULSE) {
			out &= pulseOut;
		}
	} else if (ctrl & Mos6581::VOICE_CTRL_PULSE) {
		out = pulseOut;
	} else if (ctrl & Mos6581::VOICE_CTRL_NOISE) {
		out = noiseOut;
	}
	out *= mEG.mOut;
	mVol = out >> 8;
}


void Mos6581Channel::Write(uint32_t addr, uint8_t val)
{
	uint8_t reg = addr - Mos6581::REGISTER_BASE;

	switch (reg) {
	case Mos6581::R_VOICE1_FREQ_LO:
	case Mos6581::R_VOICE2_FREQ_LO:
	case Mos6581::R_VOICE3_FREQ_LO:
		//if (mIndex==2) NLOGD("Mos6581", "FLO[2] = %#x", val);
		mStep = val | ((uint16_t)mChip->mRegs[Mos6581::R_VOICE1_FREQ_HI + mIndex * 7] << 8);
		break;

	case Mos6581::R_VOICE1_FREQ_HI:
	case Mos6581::R_VOICE2_FREQ_HI:
	case Mos6581::R_VOICE3_FREQ_HI:
		//if (mIndex==2) NLOGD("Mos6581", "FHI[2] = %#x", val);
		mStep = ((uint16_t)val << 8) | mChip->mRegs[Mos6581::R_VOICE1_FREQ_LO + mIndex * 7];
		break;

	case Mos6581::R_VOICE1_PW_LO:
	case Mos6581::R_VOICE2_PW_LO:
	case Mos6581::R_VOICE3_PW_LO:
		mDuty = val | (((uint16_t)mChip->mRegs[Mos6581::R_VOICE1_PW_HI + mIndex * 7] & 0xF) << 8);
		//if (mIndex==2) NLOGD("Mos6581", "PW = %#x", mDuty);
		break;

	case Mos6581::R_VOICE1_PW_HI:
	case Mos6581::R_VOICE2_PW_HI:
	case Mos6581::R_VOICE3_PW_HI:
		mDuty = ((uint16_t)(val & 0xF) << 8) | mChip->mRegs[Mos6581::R_VOICE1_PW_LO + mIndex * 7];
		//if (mIndex==2) NLOGD("Mos6581", "PW = %#x", mDuty);
		break;

	case Mos6581::R_VOICE1_AD:
	case Mos6581::R_VOICE2_AD:
	case Mos6581::R_VOICE3_AD:
		//if (mIndex==2) NLOGD("Mos6581", "AD for channel %d = %#x", mIndex, val);
		break;

	case Mos6581::R_VOICE1_SR:
	case Mos6581::R_VOICE2_SR:
	case Mos6581::R_VOICE3_SR:
		mEG.mSustainLevel = mChip->mRegs[Mos6581::R_VOICE1_SR + mIndex * 7] >> 4;
		mEG.mSustainLevel |= mEG.mSustainLevel << 4;
		//if (mIndex==2) NLOGD("Mos6581", "SR for channel %d = %#x", mIndex, val);
		break;

	case Mos6581::R_VOICE1_CTRL:
	case Mos6581::R_VOICE2_CTRL:
	case Mos6581::R_VOICE3_CTRL:
		//if (mIndex==2) NLOGD("Mos6581", "VOICE%d_CTRL = %#x", mIndex, val);
		if ((val ^ prevRegValue) & Mos6581::VOICE_CTRL_GATE) {
			if (val & Mos6581::VOICE_CTRL_GATE) {
				mEG.mPhase = Mos6581EnvelopeGenerator::ATTACK;
				mEG.mClockDivider = 1;
				mEG.mClocked = true;
				mEG.mPeriod = EG_PERIODS[mChip->mRegs[Mos6581::R_VOICE1_AD + mIndex * 7] >> 4];
				if (mIndex==2) NLOGV("Mos6581", "Attack phase on channel 0 at level %d (%d)", mEG.mOut, curCycle);
			} else {
				mEG.mPhase = Mos6581EnvelopeGenerator::RELEASE;
				mEG.mPeriod = EG_PERIODS[mChip->mRegs[Mos6581::R_VOICE1_SR + mIndex * 7] & 0x0F];
				mEG.mClocked = true;
				if (mIndex==2) NLOGV("Mos6581", "Channel %d starting release phase at output level %d, (period %d, divider %d, time %d)",
						mIndex, mEG.mOut, mEG.mPeriod, mEG.mClockDivider, curCycle);
			}
		}
		if (val & Mos6581::VOICE_CTRL_TEST) {
			mLfsr = 0x7FFFF8;
			mPos = 0xFFFFFF;
		}
		break;

	default:
		break;
	}
}


void Mos6581::Reset()
{
	NLOGD("Mos6581", "Reset");

	for (int i = 0; i < 3; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}
}


void Mos6581::Step()
{
	for (int i = 0; i < 3; i++) {
		mChannels[i].Step();
	}
	curCycle++;
}


void Mos6581::Write(uint32_t addr, uint8_t data)
{
	//NLOGD("Mos6581", "Write(%#x, %#x)", addr, data);

	uint8_t reg = addr - Mos6581::REGISTER_BASE;

	prevRegValue = mRegs[reg];
	mRegs[reg] = data;

	if (reg >= Mos6581::R_VOICE1_FREQ_LO && reg <= Mos6581::R_VOICE1_SR) {
		mChannels[0].Write(addr, data);

	} else if (reg >= Mos6581::R_VOICE2_FREQ_LO && reg <= Mos6581::R_VOICE2_SR) {
		mChannels[1].Write(addr, data);

	} else if (reg >= Mos6581::R_VOICE3_FREQ_LO && reg <= Mos6581::R_VOICE3_SR) {
		mChannels[2].Write(addr, data);

	} else if (Mos6581::R_FILTER_FC_LO == reg) {

	} else if (Mos6581::R_FILTER_FC_HI == reg) {

	} else if (Mos6581::R_FILTER_RESFIL == reg) {

	} else if (Mos6581::R_FILTER_MODEVOL == reg) {
		if (data & 0x80) {
			mChannels[2].mOutputMask = 0;
		} else {
			mChannels[2].mOutputMask = 0xFFFF;
		}
	}
}
