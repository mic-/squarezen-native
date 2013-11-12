/*
 * Pokey.h
 *
 *  Created on: Nov 12, 2013
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

#ifndef POKEY_H_
#define POKEY_H_

#include "Oscillator.h"

class Pokey;


class PokeyChannel : public Oscillator
{
public:
	virtual ~PokeyChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	void SetChip(Pokey *chip) { mChip = chip; }
	void SetIndex(uint32_t index) { mIndex = index; }

private:
	Pokey *mChip;
	uint32_t mClockPrescaler, mPrescalerStep;
	uint32_t mIndex;
};


class Pokey
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t val);

	enum
	{
		CLOCK_1_79_MHZ = 1789790,
		CLOCK_64_KHZ = 63921,
		CLOCK_15_KHZ = 15699,
	};

	// Register enumerators (zero-based)
	enum
	{
		R_AUDF1 = 0x00,
		R_AUDC1 = 0x01,
		R_AUDF2 = 0x02,
		R_AUDC2 = 0x03,
		R_AUDF3 = 0x04,
		R_AUDC3 = 0x05,
		R_AUDF4 = 0x06,
		R_AUDC4 = 0x07,
		R_AUDCTL = 0x08,
	};

	// AUDCTL settings
	enum
	{
		AUDCTL_NORMAL_CLOCK_MASK = 0x01,
		AUDCTL_NORMAL_CLOCK_64KHZ = 0x00,
		AUDCTL_NORMAL_CLOCK_15KHZ = 0x01,

		AUDCTL_CHN2_FILTER_MASK = 0x02,
		AUDCTL_CHN2_FILTER_OFF = 0x00,
		AUDCTL_CHN2_FILTER_ON = 0x02,

		AUDCTL_CHN1_FILTER_MASK = 0x04,
		AUDCTL_CHN1_FILTER_OFF = 0x00,
		AUDCTL_CHN1_FILTER_ON = 0x04,

		AUDCTL_CHN3_4_PAIR_MASK = 0x08,
		AUDCTL_CHN3_4_SEPARATE = 0x00,
		AUDCTL_CHN3_4_PAIRED = 0x08,

		AUDCTL_CHN1_2_PAIR_MASK = 0x10,
		AUDCTL_CHN1_2_SEPARATE = 0x00,
		AUDCTL_CHN1_2_PAIRED = 0x10,

		AUDCTL_CHN3_CLOCK_MASK = 0x20,
		AUDCTL_CHN3_CLOCK_64KHZ = 0x00,
		AUDCTL_CHN3_CLOCK_1_79MHZ = 0x20,

		AUDCTL_CHN1_CLOCK_MASK = 0x40,
		AUDCTL_CHN1_CLOCK_64KHZ = 0x00,
		AUDCTL_CHN1_CLOCK_1_79MHZ = 0x40,

		AUDCTL_POLY_BITS_MASK = 0x80,
		AUDCTL_POLY_17_BITS = 0x00,
		AUDCTL_POLY_9_BITS = 0x80,
	};

	PokeyChannel mChannels[4];

private:
	friend class PokeyChannel;
	uint8_t mRegs[16];
};

#endif /* POKEY_H_ */

