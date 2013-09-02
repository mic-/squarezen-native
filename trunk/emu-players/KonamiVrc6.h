/*
 * KonamiVrc6.h
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

#ifndef KONAMIVRC6_H_
#define KONAMIVRC6_H_

#include <stdint.h>
#include "Oscillator.h"


class KonamiVrc6Channel : public Oscillator
{
public:
	virtual ~KonamiVrc6Channel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	void SetIndex(uint8_t index) { mIndex = index; }

	bool mEnabled;
	uint16_t mWaveStep;
	uint16_t mIndex;
	uint16_t mVol;
	uint8_t mPhase;
	uint8_t mDuty;
	uint8_t mMode;
};


class KonamiVrc6
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	// Channel enumerators
	enum
	{
		CHN_PULSE1  = 0,
		CHN_PULSE2  = 1,
		CHN_SAW 	= 2,
	};

	static const uint8_t SQUARE_WAVES[8][16];

private:
	KonamiVrc6Channel mChannels[3];
};

#endif
