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

#define NLOG_LEVEL_DEBUG 0
#define NLOG_TAG "KonamiVrc6"

#include "NativeLogger.h"
#include "KonamiVrc6.h"


const uint8_t KonamiVrc6::SQUARE_WAVES[8][16] =
{
	{0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0},	// 6.25%
	{0,1,1,0,0,0,0,0, 0,0,0,0,0,0,0,0},	// 12.5%
	{0,1,1,1,0,0,0,0, 0,0,0,0,0,0,0,0},	// 18.75%
	{0,1,1,1,1,0,0,0, 0,0,0,0,0,0,0,0},	// 25%
	{0,1,1,1,1,1,0,0, 0,0,0,0,0,0,0,0},	// 31.25%
	{0,1,1,1,1,1,1,0, 0,0,0,0,0,0,0,0},	// 37.5%
	{0,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0},	// 43.75%
	{0,1,1,1,1,1,1,1, 1,0,0,0,0,0,0,0}	// 50%
};


void KonamiVrc6Channel::Reset()
{
	// ToDo: implement
	mEnabled = false;
	mAccum = 0;
	mAccumClockCount = 0;
}


void KonamiVrc6Channel::Step()
{
	if (mEnabled) mPos++;

	if (mIndex <= KonamiVrc6::CHN_PULSE2) {
		if (mPos >= mPeriod) {
			mPos = 0;
			mWaveStep &= 0x0F;
			mPhase = (mMode) ? 1 : KonamiVrc6::SQUARE_WAVES[mDuty][mWaveStep++];
			if (mPeriod < 8) mPhase = 0;
		}
	} else {
		// handle saw wave
		if (mPos >= mPeriod) {
			mPos = 0;
			if (mAccumClockCount & 1) {
				mAccum += mAccumStep;
			}
			mAccumClockCount++;
			if (mAccumClockCount == 14) {
				mAccumClockCount = 0;
				mAccum = 0;
			}
			mPhase = mAccum >> 3;
		}
	}
}


void KonamiVrc6Channel::Write(uint32_t addr, uint8_t data)
{
	switch (addr & 3) {
	case 0:	// control
		if (addr != 0xB000) {
			mVol = data = 0x0F;
			mDuty = (data >> 4) & 7;
			mMode = data & 0x80;
		} else {
			// Saw channel
			mVol = data & 0x0F;
			mAccumStep = data & 0x3F;
		}
		break;
	case 1:	// freq low
		mPeriod = (mPeriod & 0xF00) | data;
		break;
	case 2:	// freq high
		mPeriod = (mPeriod & 0xFF) | ((uint16_t)(data & 0x0F) << 8);
		mEnabled = ((data & 0x80) == 0x80);
		break;
	default:
		break;
	}
}


void KonamiVrc6::Reset()
{
	for (int i = KonamiVrc6::CHN_PULSE1; i <= KonamiVrc6::CHN_SAW; i++) {
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}
}


void KonamiVrc6::Step()
{
	for (int i = KonamiVrc6::CHN_PULSE1; i <= KonamiVrc6::CHN_SAW; i++) {
		mChannels[i].Step();
	}
}


void KonamiVrc6::Write(uint32_t addr, uint8_t data)
{
	switch (addr) {
	case 0x9000:	// VRC6 pulse1 control (MDDDVVVV)
	case 0x9001:	// VRC6 pulse1 freq low (LLLLLLLL)
	case 0x9002:	// VRC6 pulse1 freq high (E...HHHH)
		mChannels[KonamiVrc6::CHN_PULSE1].Write(addr, data);
		break;
	case 0xA000:	// VRC6 pulse2 control
	case 0xA001:	// VRC6 pulse2 freq low
	case 0xA002:	// VRC6 pulse2 freq high
		mChannels[KonamiVrc6::CHN_PULSE2].Write(addr, data);
		break;
	case 0xB000:	// VRC6 saw accumulator rate
	case 0xB001:	// VRC6 saw freq low
	case 0xB002:	// VRC6 saw freq high
		mChannels[KonamiVrc6::CHN_SAW].Write(addr, data);
		break;
	default:
		NLOGV(NLOG_TAG, "Unrecognized address: %#x", addr);
		break;
	}
}
