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
	// for (i : 0..15) YM_VOL_TB[i] = floor(power(10, (i-15)/6.67)*3840)
	21, 30, 43, 60, 86, 121, 171, 242, 342, 483, 683, 965, 1363, 1925, 2718, 3840
};

const uint16_t YmChip::YM2149_ENVE_TB[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 5, 7, 10, 15,
	21, 30, 43, 60, 86, 121, 171, 242, 342, 483, 683, 965, 1363, 1925, 2718, 3840
};

const uint32_t YmChip::TIMER_PRESCALER_TB[] = {
	0,4,10,16,
	50,64,100,200
};

const uint8_t YmSoundFX::DIGIDRUM_U8_TO_U4_TB[] = {
	0,0,1,1,3,4,4,5,5,5,6,6,6,6,7,7,
	7,7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,
	9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,
	10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,14,14,14,14,14,14,
	14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
	14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
	14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
	14,14,14,14,14,14,14,14,14,14,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
};

static uint8_t ymRegs[16];


void YmChannel::Reset()
{
	mPhase = 0;
	mOut = -1;
	mCurVol = 0;

	mMode = 0;
	mCurVol = &mVol;
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

	//NLOGV("YM2149", "EG output = %d, attack = %d, cycle = %d", mOut, mAttack, mCycle);
}

void YmEnvelopeGenerator::Write(uint32_t addr, uint8_t data)
{
	if (addr == YmChip::R_ENVE_SHAPE) {
		if (data & 4)
			mAttack = mMaxCycle;
		else
			mAttack = 0;

		if (data & 8) {
			mHold = (data & 1) ? mMaxCycle : 0;
			mAlt  = (data & 2) ? mMaxCycle : 0;
		} else {
			mHold = mMaxCycle;
			mAlt  = mAttack;
		}
		mCycle = mMaxCycle;
		mOut = mEnvTable[mAttack ^ mCycle];
	} else if (addr == YmChip::R_ENVE_FREQL) {
		mPeriodPremult = (mPeriodPremult & 0xFF00) | data;
		mPeriod = mPeriodPremult * ((mMaxCycle == 15) ? 16 : 8);
	} else if (addr == YmChip::R_ENVE_FREQH) {
		mPeriodPremult = (mPeriodPremult & 0xFF) | ((uint32_t)(data) << 8);
		mPeriod = mPeriodPremult * ((mMaxCycle == 15) ? 16 : 8);
	}
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


void YmChannel::Write(uint32_t addr, uint8_t data)
{
	switch (addr) {
	case 0:
	case 2:
	case 4:
		mPeriodPremult = (mPeriodPremult & 0xF00) | data;
		mPeriod = mPeriodPremult * 8;
		break;
	case 1:
	case 3:
	case 5:
		mPeriodPremult = (mPeriodPremult & 0xFF) | ((uint16_t)(data & 0x0F) << 8);
		mPeriod = mPeriodPremult * 8;
		break;
	case YmChip::R_LEVEL_A:
	case YmChip::R_LEVEL_B:
	case YmChip::R_LEVEL_C:
		mMode = data & 0x10;
		mVol = YmChip::YM2149_VOL_TB[data & 0x0F];
		mCurVol = mMode ? &(mChip->mEG.mOut) : &mVol;
		mChip->mEG.mHalt = (mChip->mChannels[0].mMode |
				 	 	    mChip->mChannels[1].mMode |
				 	 	 	mChip->mChannels[2].mMode) ^ 0x10;
		break;
	case YmChip::R_MIXER:
		mToneOff = (data >> mIndex) & 1;
		mNoiseOff = (data >> (mIndex + 3)) & 1;
		break;
	}
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
	Write(6, regs[6]);
}

void YmNoise::Write(uint32_t addr, uint8_t data)
{
	if (addr == 6) {
		mPeriod = (uint32_t)(data & 0x1F) * 8;
	}
}

void YmSoundFX::Reset()
{
	mPeriod = 0;
	mEffectChannel = 0;
	mType = SFX_NONE;
}


void YmSoundFX::Step()
{
	if (mPeriod) {
		mPos++;
		if (mPos >= mPeriod) {
			mPos = 0; //-= mPeriod;
			if (mType == SFX_SID_VOICE) {
				mChip->mChannels[mEffectChannel - 1].Write(YmChip::R_LEVEL_A + mEffectChannel - 1, mVol);
				mVol ^= mVMax;

			} else if (mType == SFX_SYNC_BUZZER) {
				mChip->mEG.Write(YmChip::R_ENVE_SHAPE, mVol);

			} else if (mType == SFX_DIGI_DRUM) {
				if (mDigiDrumPos < mChip->mDigiDrumLen[mDigiDrumSample]) {
					if (YmChip::DIGI_DRUM_U4 == mChip->mDigiDrumFormat) {
						mVol = (mChip->mDigiDrumPtr[mDigiDrumSample])[mDigiDrumPos] & 0x0F;
					} else {
						mVol = DIGIDRUM_U8_TO_U4_TB[(mChip->mDigiDrumPtr[mDigiDrumSample])[mDigiDrumPos] & 0xFF];
					}
					mChip->mChannels[mEffectChannel - 1].Write(YmChip::R_LEVEL_A + mEffectChannel - 1, mVol);
					mDigiDrumPos++;
				} else {
					NLOGE("YM2149", "Reached end of sample; stopping digidrum");
					mChip->mChannels[mEffectChannel - 1].mSfxActive &= ~(1 << SFX_DIGI_DRUM);
					mPeriod = 0;
					mType = SFX_NONE;
				}
			}
		}
	}
}


void YmSoundFX::Write(uint8_t *regs)
{
	int effectChannel;
	YmChannel *chn;
	static uint32_t frame = 0;

	uint8_t prevType = mType;
	uint8_t prevChannel = mEffectChannel;

	mType = 0;

	if (YM_FORMAT_6 == mFormat) {
		// YM6 supports 4 different effects; two of which can be active at the same time.
		// r1[7:6] selects the type of effect to use for effect 1, and r1[5:4] selects the channel to apply it on.
		// r3[7:6] selects the type of effect to use for effect 2, and r3[5:4] selects the channel to apply it on.
		effectChannel = (regs[1 + mIndex*2] >> 4) & 3;
		if (effectChannel) {
			mType = (regs[1 + mIndex*2] >> 6) & 3;
		}
	} else {
		// YM5 and below only support SID-Voice and Digidrum.
		// r1,r6,r14 controls the SID-voice effect, and r3,r8,r15 controls the Digidrum effect
		effectChannel = (regs[1 + mIndex*2] >> 4) & 3;
		if (effectChannel) {
			mType = (0 == mIndex) ? SFX_SID_VOICE : SFX_DIGI_DRUM;
		}
	}

	if (effectChannel) {
		mEffectChannel = effectChannel;
		chn = &(mChip->mChannels[effectChannel - 1]);
		chn->mSfxActive = (1 << mType);

		if (SFX_SID_VOICE == mType) {
			if (!mPeriod || (mVol != 0 && ymRegs[YmChip::R_LEVEL_A + effectChannel - 1] != mVMax)) {
				mVol = ymRegs[YmChip::R_LEVEL_A + effectChannel - 1];
			}
			mVMax = ymRegs[YmChip::R_LEVEL_A + effectChannel - 1];
			if (mPeriod != 0 && YM_FORMAT_6 != mFormat && (regs[1] & 0x40)) {
				// YM5 and below supports resetting the SID-Voice timer by setting r1[6]
				mPos = 0;
			}
			mPeriod = (YmChip::TIMER_PRESCALER_TB[regs[6 + mIndex*2] >> 5] * (uint32_t)regs[14 + mIndex] * 13) >> 4;

		} else if (SFX_SYNC_BUZZER == mType) {
			mVol = ymRegs[YmChip::R_LEVEL_A + effectChannel - 1];
			mPeriod = (YmChip::TIMER_PRESCALER_TB[regs[6 + mIndex*2] >> 5] * (uint32_t)regs[14 + mIndex] * 13) >> 4;

		} else if (SFX_DIGI_DRUM == mType) {
			if (!mPeriod) {
				mDigiDrumSample = regs[YmChip::R_LEVEL_A + chn->mIndex];
				mDigiDrumPos = 0;
			}
			mPeriod = (YmChip::TIMER_PRESCALER_TB[regs[6 + mIndex*2] >> 5] * (uint32_t)regs[14 + mIndex] * 13) >> 4;
			NLOGE("YM2149", "Starting Digidrum on channel %d at frame %d with period %d (%d Hz)", chn->mIndex, frame, mPeriod, 2000000/mPeriod);
		} else {
			// ToDo: handle Sinus-SID
			NLOGD("YM2149", "Using an unsupported effect: %d", mType);
			mPeriod = 0;
		}
	} else {
		if (prevType & SFX_DIGI_DRUM) {
			// Let the digidrum effect run until the end of the sample has been reached
			mChip->mChannels[prevChannel - 1].mSfxActive = (1 << SFX_DIGI_DRUM);
			mType = prevType;
		} else {
			mPeriod = 0;
		}
	}

	frame++;
}


YmChip::YmChip(int16_t envelopeSteps)
{
	if (envelopeSteps != AY_3_8910_ENVELOPE_STEPS
			&& envelopeSteps != YM2149_ENVELOPE_STEPS) {
		envelopeSteps = YM2149_ENVELOPE_STEPS;
	}
	mEG.mMaxCycle = envelopeSteps - 1;
	mEG.mEnvTable = ((envelopeSteps == AY_3_8910_ENVELOPE_STEPS) ? (uint16_t*)YM2149_VOL_TB : (uint16_t*)YM2149_ENVE_TB);
}


void YmChip::Reset()
{
	mDigiDrumFormat = DIGI_DRUM_UNKNOWN;

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

	// Mark all channels as not using any effects
	for (int i = 0; i < 3; i++) {
		mChannels[i].mSfxActive = YmSoundFX::SFX_NONE;
	}

	mSfx[0].Step();
	mSfx[1].Step();
}


void YmChip::Write(uint8_t *regs)
{
	memcpy(ymRegs, regs, 16);
	mChannels[0].Write(regs);
	mChannels[1].Write(regs);
	mChannels[2].Write(regs);
	mEG.Write(regs);
	mNoise.Write(regs);
	mSfx[0].Write(regs);
	mSfx[1].Write(regs);
}


void YmChip::Write(uint32_t addr, uint8_t data)
{
	ymRegs[addr & 0x1F] = data;
	mChannels[0].Write(addr, data);
	mChannels[1].Write(addr, data);
	mChannels[2].Write(addr, data);
	if (addr >= R_ENVE_FREQL && addr <= R_ENVE_SHAPE) {
		mEG.Write(addr, data);
	} else if (addr == 6) {
		mNoise.Write(addr, data);
	}
}


