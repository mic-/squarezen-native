/*
 * YM2149.h
 *
 *  Created on: May 23, 2013
 *      Author: Mic
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

	void SetChip(YmChip *chip) { mChip = chip; }
	void SetIndex(uint16_t index) { mIndex = index; }

	YmChip *mChip;
	uint16_t *mCurVol;
	uint16_t mIndex;
	uint16_t mOut;
	uint16_t mVol;
	uint8_t mMode;
	uint8_t mToneOff, mNoiseOff;
	uint8_t mChnIndex;
	uint8_t mPhase;
};


class YmNoise : public Oscillator
{
public:
	virtual ~YmNoise() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t *regs);

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

	void SetChip(YmChip *chip) { mChip = chip; }

	YmChip *mChip;
	uint16_t *mEnvTable;
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

	enum {
		SFX_SID_VOICE = 0,
		SFX_DIGI_DRUM = 1,
	};

	YmChip *mChip;
	uint32_t mDigiDrumPos, mDigiDrumLen;
	uint8_t *mDigiDrumSample;
	uint16_t mIndex;
	uint8_t mType;
	uint16_t mVol, mVMax;
};


class YmChip
{
public:
	void Reset();
	void Step();
	void Write(uint8_t *regs);

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

	YmChannel mChannels[3];
	YmNoise mNoise;
	YmEnvelopeGenerator mEG;
	YmSoundFX mSfx[2];
	uint8_t *mDigiDrumPtr[16];
	uint32_t mDigiDrumLen[16];
};


#endif /* YM2149_H_ */
