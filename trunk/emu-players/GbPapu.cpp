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

#include "GbPapu.h"

const uint8_t GbPapuChip::SQUARE_WAVES[4][32] =
{
		// 12.5%
		{1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
		// 25%
		{1,1,1,1, 1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
		// 50%
		{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
		// 75%
		{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 0,0,0,0, 0,0,0,0}
};

void GbPapuChannel::Reset()
{
	// TODO: fill out
	mWaveStep = 0;
	mPhase = 0;
	mDuty = 0;
}


void GbPapuChannel::Step()
{
	// TODO: fill out
}


void GbPapuChannel::Write(uint32_t addr, uint8_t val)
{
	// TODO: fill out
}


void GbPapuChip::Reset()
{
	for (int i = 0; i < 4; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}
}

void GbPapuChip::Write(uint32_t addr, uint8_t val)
{
	if (addr >= 0xFF10 && addr <= 0xFF14) {
		mChannels[0].Write(addr, val);

	} else if (addr >= 0xFF16 && addr <= 0xFF19) {
		mChannels[1].Write(addr, val);

	} else if (addr >= 0xFF1A && addr <= 0xFF1E) {
		mChannels[2].Write(addr, val);

	} else if (addr >= 0xFF20 && addr <= 0xFF23) {
		mChannels[3].Write(addr, val);
	}
}

