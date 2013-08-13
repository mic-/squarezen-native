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

#include "SN76489.h"


const uint16_t SnChip::SN76489_VOL_TB[16] = {
	3840, 2718, 1925, 1363, 965, 683, 483, 342,
	242, 171, 121, 86, 60, 43, 30, 0
};


void SnChannel::Reset()
{
	mPhase = 0;
	mLfsr = 0x01;
}


void SnChannel::Step()
{
	mPos++;
	if (mIndex != SnChip::CHN_NOISE) {
		// Tone
		if (mPos >= mPeriod*16) {
			mPos = 0;
			mPhase ^= 1;
		}
	} else {
		// Noise
		uint32_t period = 0x10 << (mPeriod & NOISE_PERIOD_MASK);
		if ((mPeriod & NOISE_PERIOD_MASK) == NOISE_PERIOD_TONE2) {
			period = mChip->mChannels[2].mPeriod;
		}
		period <<= 4;

		if (mPos >= period) {
			mPos -= period;
			mPhase  = mLfsr & 1;
			if ((mPeriod & NOISE_TYPE_MASK) == NOISE_TYPE_WHITE) {
				mLfsr = (mLfsr >> 1) | ((uint32_t)(mPhase ^ ((mLfsr >> 3) & 1)) << 15);
			} else {
				mLfsr = (mLfsr >> 1) | ((mLfsr & 1) << 15);
			}
		}
	}
}


void SnChannel::Write(uint32_t addr, uint8_t val)
{
	//NativeLog(0, "SN76489", "Writing %#x to PSG", val);

	switch (addr) {
	case 0x06:
		break;

	case 0x7F:
		if (val & SnChip::CMD_LATCH_MASK) {
			if ((val & SnChip::CMD_TYPE_MASK) == SnChip::CMD_TYPE_FREQ) {
				mPeriod &= 0x3F0;
				mPeriod |= (val & 0x0F);
			} else {
				mVol &= 0x3F0;
				mVol |= (val & 0x0F);
			}
		} else {
			if ((mChip->mLatchedByte & SnChip::CMD_TYPE_MASK) == SnChip::CMD_TYPE_FREQ) {
				mPeriod &= 0x00F;
				mPeriod |= (uint16_t)(val & 0x3F) << 4;
			} else {
				mVol &= 0x00F;
				mVol |= (uint16_t)(val & 0x3F) << 4;
			}
		}
		break;
	}
}



void SnChip::Reset()
{
	for (int i = SnChip::CHN_TONE0; i <= SnChip::CHN_NOISE; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}
}


void SnChip::Step()
{
	for (int i = SnChip::CHN_TONE0; i <= SnChip::CHN_NOISE; i++) {
		mChannels[i].Step();
	}
}


void SnChip::Write(uint8_t addr, uint8_t val)
{
	if (val & CMD_LATCH_MASK) {
		mChannels[(val >> 5) & 3].Write(addr, val);
		mLatchedByte = val;
	} else {
		mChannels[(mLatchedByte >> 5) & 3].Write(addr, val);
	}
}
