/*
 * Sunsoft5B.h
 *
 *  Created on: Sep 1, 2013
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

#ifndef SUNSOFT5B_H_
#define SUNSOFT5B_H_

#include "Oscillator.h"
#include "YM2149.h"

class Sunsoft5B;

class Sunsoft5BChannel : public Oscillator
{
public:
	virtual ~Sunsoft5BChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	void SetChip(Sunsoft5B *chip) { mChip = chip; }
	void SetIndex(uint16_t index) { mIndex = index; }

	Sunsoft5B *mChip;
	uint16_t *mCurVol;
	uint16_t mIndex;
	uint16_t mOut;
	uint16_t mVol;
	uint8_t mMode;
	uint8_t mToneOff, mNoiseOff;
	uint8_t mChnIndex;
	uint8_t mPhase;
};


class Sunsoft5BNoise : public YmNoise
{
public:
	virtual ~Sunsoft5BNoise() {}

	virtual void Write(uint32_t addr, uint8_t val);
};


class Sunsoft5BEnvelopeGenerator : public YmEnvelopeGenerator
{
public:
	virtual ~Sunsoft5BEnvelopeGenerator() {}

	virtual void Write(uint32_t addr, uint8_t val);
	uint32_t mPeriodPremult;
};


class Sunsoft5B
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t val);

	static const uint16_t SUNSOFT5B_VOL_TB[16];
	static const uint16_t SUNSOFT5B_ENVE_TB[32];

	Sunsoft5BChannel mChannels[3];
	Sunsoft5BNoise mNoise;
	Sunsoft5BEnvelopeGenerator mEG;
};


#endif /* SUNSOFT5B_H_ */
