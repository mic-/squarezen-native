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


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "YM2149.h"

#ifdef __TIZEN__
#include <FBase.h>
#endif


//#define LOG_PCM 1


const uint16_t YmChip::YM2149_VOL_TB[] = {
	// for (i : 0..15) YM_VOL_TB[i] = floor(power(10, (i-15)/6.67)*82)
	0x0000,0x0000,0x0000,0x0001,
	0x0001,0x0002,0x0003,0x0005,
	0x0007,0x000A,0x000E,0x0014,
	0x001D,0x0029,0x003A,0x0052
};

const uint16_t YmChip::YM2149_ENVE_TB[] = {
	0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0001,
	0x0001,0x0001,0x0001,0x0002,
	0x0002,0x0003,0x0003,0x0004,
	0x0005,0x0006,0x0007,0x0008,
	0x0009,0x000B,0x000C,0x000E,
	0x0011,0x0013,0x0015,0x0019,
	0x001D,0x0029,0x003A,0x0052
};

const uint32_t YmChip::TIMER_PRESCALER_TB[] = {
	0,4,10,16,
	50,64,100,200
};




void YmChannel::Reset()
{
	mPhase = 0;
	mOut = -1;
	mCurVol = 0;
}


void YmEnvelopeGenerator::Reset()
{
	mHold = mAlt = mAttack = 0;
}

void YmNoise::Reset()
{
	mLfsr = 0x10000;
}


void YmEnvelopeGenerator::Step()
{
	if (!mHalt) {
		mPos++;
		if (mPos >= mPeriod) {
			mPos -= mPeriod;
			mCycle--;
			if (mCycle < 0) {
				mAttack ^= mAlt;
				mCycle &= (mHold ^ mMaxCycle);
				mHalt = mHold;
			}
			mOut = mEnvTable[mCycle ^ mAttack];
		}
	}
}


void YmEnvelopeGenerator::Write(uint8_t *regs)
{
	if (regs[YmChip::R_ENVE_SHAPE] != 0xFF) {
		if (regs[YmChip::R_ENVE_SHAPE] & 4)
			mAttack = mMaxCycle;
		else
			mAttack = 0;

		if (regs[YmChip::R_ENVE_SHAPE] & 8) {
			mHold = (regs[YmChip::R_ENVE_SHAPE] & 1) ? mMaxCycle : 0;
			mAlt  = (regs[YmChip::R_ENVE_SHAPE] & 2) ? mMaxCycle : 0;
		} else {
			mHold = mMaxCycle;
			mAlt  = mAttack;
		}
		mCycle = mMaxCycle;
		mOut = mEnvTable[mAttack ^ mCycle];
	}

	mPeriod = regs[YmChip::R_ENVE_FREQL];
	mPeriod |= (uint32_t)(regs[YmChip::R_ENVE_FREQH]) << 8;
	mPeriod *= (mMaxCycle == 15) ? 16 : 8;

	mHalt = (mChip->mChannels[0].mMode |
			 mChip->mChannels[1].mMode |
			 mChip->mChannels[2].mMode) ^ 0x10;

	//AppLog("EG output = %d, attack = %d, cycle = %d", mOut, mAttack, mCycle);

}


void YmChannel::Step()
{
	mPos++;

	if (mPos >= mPeriod) {
		mPos -= mPeriod;
		mPhase ^= 1;
	}
}


void YmChannel::Write(uint8_t *regs)
{
	mMode     = regs[YmChip::R_LEVEL_A + mIndex] & 0x10;
	mToneOff  = (regs[YmChip::R_MIXER] >> mIndex) & 1;
	mNoiseOff = (regs[YmChip::R_MIXER] >> (mIndex + 3)) & 1;
	mVol      = YmChip::YM2149_VOL_TB[regs[YmChip::R_LEVEL_A + mIndex] & 0x0F];

	mPeriod = regs[mIndex * 2];
	mPeriod |= (uint32_t)(regs[mIndex * 2 + 1] & 0x0F) << 8;
	mPeriod *= 8;

	mCurVol = mMode ? &(mChip->mEG.mOut) : &mVol;
}


void YmNoise::Step()
{
	mPos++;
	if (mPos >= mPeriod) {
		mPos -= mPeriod;
		mOut  = mLfsr & 1;
		mLfsr = (mLfsr >> 1) | ((uint32_t)(mOut ^ ((mLfsr >> 3) & 1)) << 16);
	}
}


void YmNoise::Write(uint8_t *regs)
{
	mPeriod = (uint32_t)(regs[6] & 0x1F) * 8;
}


void YmSoundFX::Reset()
{

}


void YmSoundFX::Step()
{
	if (mPeriod) {
		mPos++;
		if (mPos >= mPeriod) {
			mPos -= mPeriod;
			if (mType == SFX_SID_VOICE) {
				mVol ^= mVMax;
			} else if (mType == SFX_DIGI_DRUM) {
				if (mDigiDrumPos < mDigiDrumLen) {
					mVol = YmChip::YM2149_VOL_TB[mDigiDrumSample[mDigiDrumPos] & 0x0F];
					mDigiDrumPos++;
				}
			}
		}
	}
}


void YmSoundFX::Write(uint8_t *regs)
{
	int i;
	YmChannel *chn;

	// TODO: Handle SFX #2

	mPeriod = 0;
	i = regs[1] & 0x30;
	if (i) {
		chn = &(mChip->mChannels[i >> 4]);

		mType = regs[1] >> 6;
		// TODO: Handle other effects than SID Voice / Digidrum
		if (mType == SFX_SID_VOICE || mType == SFX_DIGI_DRUM) {
			mPeriod = (YmChip::TIMER_PRESCALER_TB[regs[6] >> 5] * (uint32_t)regs[14] * 13) >> 4;
			chn->mCurVol = &mVol;

			if (mType == SFX_SID_VOICE) {
				mVol = chn->mVol;
				mVMax = mVol;
			} else if (mType == SFX_DIGI_DRUM) {
				mVol = regs[YmChip::R_LEVEL_A + chn->mChnIndex];
				mVol >>= 4;
				mDigiDrumSample = mChip->mDigiDrumPtr[mVol];
				mDigiDrumLen = mChip->mDigiDrumLen[mVol];
				mVol = YmChip::YM2149_VOL_TB[mDigiDrumSample[0]];
				mDigiDrumPos = 1;
			}
		}
	}
}


void YmChip::Reset()
{
	for (int i = 0; i < 3; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}

	mEG.SetChip(this);
	mEG.Reset();
	mNoise.Reset();

	for (int i = 0; i < 2; i++) {
		mSfx[i].SetChip(this);
		mSfx[i].SetIndex(i);
		mSfx[i].Reset();
	}
}


void YmChip::Step()
{
	mChannels[0].Step();
	mChannels[1].Step();
	mChannels[2].Step();
	mEG.Step();
	mNoise.Step();
	mSfx[0].Step();
	mSfx[1].Step();
}


void YmChip::Write(uint8_t *regs)
{
	mChannels[0].Write(regs);
	mChannels[1].Write(regs);
	mChannels[2].Write(regs);
	mEG.Write(regs);
	mNoise.Write(regs);
	mSfx[0].Write(regs);
	mSfx[1].Write(regs);
}



