/*
 * YM2149.h
 *
 *  Created on: May 23, 2013
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

#ifndef YM2149_H_
#define YM2149_H_

#include "Oscillator.h"

class YmChip;

class YmChannel : public Oscillator
{
public:
	virtual ~YmChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t *regs);
	virtual void Write(uint32_t addr, uint8_t data);

	void SetChip(YmChip *chip) { mChip = chip; }
	void SetIndex(uint16_t index) { mIndex = index; }

	YmChip *mChip;
	uint32_t mPeriodPremult;
	uint16_t *mCurVol;
	uint16_t mIndex;
	uint16_t mOut;
	uint16_t mVol;
	uint8_t mMode;
	uint8_t mToneOff, mNoiseOff;
	//uint8_t mChnIndex;
	uint8_t mSfxActive;
	uint8_t mPhase;
};


class YmNoise : public Oscillator
{
public:
	virtual ~YmNoise() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t *regs);
	virtual void Write(uint32_t addr, uint8_t data);

	uint32_t mLfsr;
	uint16_t mOut;
};


class YmEnvelopeGenerator : public Oscillator
{
public:
	virtual ~YmEnvelopeGenerator() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t *regs);
	virtual void Write(uint32_t addr, uint8_t data);

	virtual void SetChip(YmChip *chip) { mChip = chip; }

	YmChip *mChip;
	uint16_t *mEnvTable;
	uint32_t mPeriodPremult;
	int16_t mCycle, mMaxCycle;
	uint16_t mOut;
	uint8_t mAttack, mAlt;
	uint8_t mHold, mHalt;
};


class YmSoundFX : public Oscillator
{
public:
	virtual ~YmSoundFX() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t *regs);

	void SetChip(YmChip *chip) { mChip = chip; }
	void SetIndex(uint16_t index) { mIndex = index; }
	void SetYmFormat(uint16_t fmt) { mFormat = fmt; }

	enum {
		SFX_SID_VOICE = 0,
		SFX_DIGI_DRUM = 1,
		SFX_SINUS_SID = 2,
		SFX_SYNC_BUZZER = 3,
		SFX_NONE = 100,
	};

	enum {
		YM_FORMAT_UNKNOWN = 0,
		YM_FORMAT_5 = 5,
		YM_FORMAT_6 = 6,
	};

	YmChip *mChip;
	uint32_t mDigiDrumPos, mDigiDrumLen;
	uint8_t *mDigiDrumSample;
	uint16_t mIndex;
	uint16_t mFormat;
	uint8_t mType;
	bool mActivated;
	uint16_t mVol, mVMax;
};


class YmChip
{
public:
	YmChip() {}
	YmChip(int16_t envelopeSteps);

	virtual ~YmChip() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t *regs);
	virtual void Write(uint32_t addr, uint8_t data);

	static const uint16_t YM2149_VOL_TB[16];
	static const uint16_t YM2149_ENVE_TB[32];
	static const uint32_t TIMER_PRESCALER_TB[16];

	enum
	{
		R_MIXER = 7,
		R_LEVEL_A = 8,
		R_LEVEL_B =	9,
		R_LEVEL_C =	10,
		R_ENVE_FREQL = 11,
		R_ENVE_FREQH = 12,
		R_ENVE_SHAPE = 13,
	};

	enum
	{
		AY_3_8910_ENVELOPE_STEPS = 16,
		YM2149_ENVELOPE_STEPS = 32,
	};

	YmChannel mChannels[3];
	YmNoise mNoise;
	YmEnvelopeGenerator mEG;
	YmSoundFX mSfx[2];
	uint8_t *mDigiDrumPtr[16];
	uint32_t mDigiDrumLen[16];
};


#endif /* YM2149_H_ */
