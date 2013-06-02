/*
 * Emu2A03.h
 *
 *  Created on: Jun 2, 2013
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

#ifndef EMU2A03_H_
#define EMU2A03_H_

#include <stdint.h>
#include "Oscillator.h"


class Emu2A03Channel : public Oscillator
{
public:
	virtual ~Emu2A03Channel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	int16_t mOut;
	uint16_t mVol, mCurVol;
	uint8_t mPhase;
};


class Emu2A03
{
public:
	void Step();

	static const uint16_t VOL_TB[];

	Emu2A03Channel mChannels[4];
};


#endif /* EMU2A03_H_ */
