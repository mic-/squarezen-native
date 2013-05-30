/*
 * GbPapu.h
 *
 *  Created on: May 29, 2013
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

#ifndef GBPAPU_H_
#define GBPAPU_H_

#include "Oscillator.h"

class GbPapuChip;


class GbPapuChannel : public Oscillator
{
public:
	virtual ~GbPapuChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t addr, uint8_t val);

	void SetChip(GbPapuChip *chip) { mChip = chip; }
	void SetIndex(uint8_t index) { mIndex = index; }

	GbPapuChip *mChip;
	uint8_t mIndex;
};


class GbPapuChip
{
public:
	void Reset();
	void Step();
	void Write(uint8_t addr, uint8_t val);

	static const uint8_t SQUARE_WAVES[4][32];

	GbPapuChannel mChannels[4];
};

#endif /* GBPAPU_H_ */
