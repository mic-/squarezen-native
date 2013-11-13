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
#define NLOG_TAG "Pokey"

#include <string.h>
#include <stddef.h>
#include "NativeLogger.h"
#include "Pokey.h"

void PokeyChannel::Reset()
{
	// ToDo: implement
}


void PokeyChannel::Step()
{
	// ToDo: implement

	mPrescalerStep++;
	if (mPrescalerStep >= mClockPrescaler) {
		mPrescalerStep = 0;
		mPos++;
		if (mPos >= mPeriod + 1) {
			mPos = 0;
			if (mIndex == 0 && (mChip->mRegs[Pokey::R_AUDCTL] & Pokey::AUDCTL_CHN1_2_PAIR_MASK) == Pokey::AUDCTL_CHN1_2_PAIRED) {
				mChip->mChannels[1].Step();
			} else if (mIndex == 2 && (mChip->mRegs[Pokey::R_AUDCTL] & Pokey::AUDCTL_CHN3_4_PAIR_MASK) == Pokey::AUDCTL_CHN3_4_PAIRED) {
				mChip->mChannels[3].Step();
			}
		}
	}
}


void PokeyChannel::Write(uint32_t addr, uint8_t data)
{
	// ToDo: implement
}


void Pokey::Reset()
{
	for (int i = 0; i < 4; i++) {
		mChannels[i].Reset();
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
	}
}


void Pokey::Step()
{
	for (int i = 0; i < 4; i++) {
		if (i == 1 && (mRegs[Pokey::R_AUDCTL] & Pokey::AUDCTL_CHN1_2_PAIR_MASK) == Pokey::AUDCTL_CHN1_2_PAIRED)
			continue;
		else if (i == 3 && (mRegs[Pokey::R_AUDCTL] & Pokey::AUDCTL_CHN3_4_PAIR_MASK) == Pokey::AUDCTL_CHN3_4_PAIRED)
			continue;
		mChannels[i].Step();
	}
}


void Pokey::Write(uint32_t addr, uint8_t data)
{
	if (addr < 0xD200 || addr > 0xD2FF)
		return;

	addr &= 0x0F;

	switch (addr) {
	case Pokey::R_AUDF1:
	case Pokey::R_AUDF2:
	case Pokey::R_AUDF3:
	case Pokey::R_AUDF4:
		break;

	case Pokey::R_AUDC1:
	case Pokey::R_AUDC2:
	case Pokey::R_AUDC3:
	case Pokey::R_AUDC4:
		break;

	case Pokey::R_AUDCTL:
		break;
	}
}
