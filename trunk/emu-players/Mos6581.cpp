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

#include "NativeLogger.h"
#include "Mos6581.h"

static const uint32_t ATTACK_PERIODS[] = {7, 31, 62, 93, 148, 218, 265, 312, 390, 976, 1953, 3125, 3906, 11718, 19531, 31250};
static const uint32_t DECAY_PERIODS[] = {23, 93, 187, 281, 445, 656, 796, 937, 1171, 2929, 5859, 9375, 11718, 35156, 58593, 93750};
static uint8_t prevRegValue;


void Mos6581EnvelopeGenerator::Reset()
{
	mPhase = ATTACK;
	mClocked = false;
	mOut = 0;
}

void Mos6581EnvelopeGenerator::Step()
{
	if (mClocked) {
		mPos++;
		if (mPos >= mPeriod) {
			mPos = 0;
			switch (mPhase) {
			case ATTACK:
				mOut++;
				if (mOut == 0xFF) {
					NLOGD("Mos6581", "Channel %d starting decay phase", mChannel->mIndex);
					mPhase = DECAY;
					mPeriod = DECAY_PERIODS[mChannel->mChip->mRegs[Mos6581::R_VOICE1_AD + mChannel->mIndex * 7] & 0x0F];
				}
				break;
			case DECAY:
				if (mOut == mSustainLevel) {
					mClocked = false;
					mPhase = SUSTAIN;
					NLOGD("Mos6581", "Channel %d starting sustain phase at level", mChannel->mIndex, mOut);
				} else {
					mOut--;
				}
				break;
			case SUSTAIN:
				mPhase = RELEASE;
				break;
			case RELEASE:
				if (mOut) mOut--;
				break;
			default:
				break;
			}
		}
	}
}


void Mos6581Mixer::Reset(uint16_t mixLevel)
{
	mMixLevel = mixLevel;
	mNumSamples = 0;
}

void Mos6581Mixer::AddSample(uint16_t sample)
{
	if (mNumSamples < 4) {
		mSamples[mNumSamples++] = sample;
	}
}

uint16_t Mos6581Mixer::Mix()
{
	uint32_t out = 0;
	if (mNumSamples) {
		out = mSamples[0];
	}
	for (int i = 1; i < mNumSamples; i++) {
		out &= mSamples[i];
	}
	out *= mMixLevel;	// 12bit * 8bit -> 20bit
	return (out >> 16) * 5;
}


void Mos6581Channel::Reset()
{
	NLOGD("Mos6581Channel", "Reset");

	mEG.SetChannel(this);
	mEG.Reset();

	mPeriod = 0;
	mStep = 0;
	mDuty = 0;
	mOut = -1;
}


void Mos6581Channel::Step()
{
	uint8_t ctrl = mChip->mRegs[Mos6581::R_VOICE1_CTRL + mIndex * 7];
	uint16_t triOut = 0, sawOut = 0, pulseOut = 0;

	if (!(ctrl & Mos6581::VOICE_CTRL_TEST)) {
		mPos = (mPos + mStep) & 0xFFFFFF;
	}

	if ((mPos >> 12) >= (4095 - mDuty)) {
		pulseOut = 0xFFF;
	}

	triOut = ((mPos >> 12) & 0x7FF) << 1;
	if (mPos & 0x800000) {
		triOut ^= 0xFFE;
	}

	sawOut = mPos >> 12;

	mEG.Step();
	mMixer.Reset(mEG.mOut);

	if (ctrl & Mos6581::VOICE_CTRL_TRIANGLE)
		mMixer.AddSample(triOut);
	if (ctrl &  Mos6581::VOICE_CTRL_SAW)
		mMixer.AddSample(sawOut);
	if (ctrl &  Mos6581::VOICE_CTRL_PULSE)
		mMixer.AddSample(pulseOut);

	mVol = mMixer.Mix();
}


void Mos6581Channel::Write(uint32_t addr, uint8_t val)
{
	uint8_t reg = addr - Mos6581::REGISTER_BASE;

	switch (reg) {
	case Mos6581::R_VOICE1_FREQ_LO:
	case Mos6581::R_VOICE2_FREQ_LO:
	case Mos6581::R_VOICE3_FREQ_LO:
		mStep = val | ((uint16_t)mChip->mRegs[Mos6581::R_VOICE1_FREQ_HI + mIndex * 7] << 8);
		break;

	case Mos6581::R_VOICE1_FREQ_HI:
	case Mos6581::R_VOICE2_FREQ_HI:
	case Mos6581::R_VOICE3_FREQ_HI:
		mStep = ((uint16_t)val << 8) | mChip->mRegs[Mos6581::R_VOICE1_FREQ_LO + mIndex * 7];
		//NLOGD("Mos6581", "F = %d on channel %d", mStep, mIndex);
		break;

	case Mos6581::R_VOICE1_PW_LO:
	case Mos6581::R_VOICE2_PW_LO:
	case Mos6581::R_VOICE3_PW_LO:
		mDuty = val | (((uint16_t)mChip->mRegs[Mos6581::R_VOICE1_PW_HI + mIndex * 7] & 0xF) << 8);
		//NLOGD("Mos6581", "PW = %#x", mDuty);
		break;

	case Mos6581::R_VOICE1_PW_HI:
	case Mos6581::R_VOICE2_PW_HI:
	case Mos6581::R_VOICE3_PW_HI:
		mStep = ((uint16_t)(val & 0xF) << 8) | mChip->mRegs[Mos6581::R_VOICE1_PW_LO + mIndex * 7];
		break;

	case Mos6581::R_VOICE1_SR:
	case Mos6581::R_VOICE2_SR:
	case Mos6581::R_VOICE3_SR:
		mEG.mSustainLevel = mChip->mRegs[Mos6581::R_VOICE1_SR + mIndex * 7] & 0xF0;
		break;

	case Mos6581::R_VOICE1_CTRL:
	case Mos6581::R_VOICE2_CTRL:
	case Mos6581::R_VOICE3_CTRL:
		if ((val ^ prevRegValue) & Mos6581::VOICE_CTRL_GATE) {
			if (val & Mos6581::VOICE_CTRL_GATE) {
				mEG.Reset();
				mEG.mClocked = true;
				mEG.mPeriod = ATTACK_PERIODS[mChip->mRegs[Mos6581::R_VOICE1_AD + mIndex * 7] >> 4];
			} else {
				NLOGD("Mos6581", "Channel %d starting release phase at output level %d", mIndex, mEG.mOut);
				mEG.mPhase = Mos6581EnvelopeGenerator::RELEASE;
				mEG.mPeriod = DECAY_PERIODS[mChip->mRegs[Mos6581::R_VOICE1_SR + mIndex * 7] & 0x0F];
				mEG.mClocked = true;
			}
		}
		if (val & Mos6581::VOICE_CTRL_TEST) {
			mPos = 0xFFFFFF;
		}
		break;

	default:
		break;
	}
}


void Mos6581::Reset()
{
	NLOGD("Mos6581", "Reset");

	for (int i = 0; i < 3; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}
}


void Mos6581::Step()
{
	for (int i = 0; i < 3; i++) {
		mChannels[i].Step();
	}
}


void Mos6581::Write(uint32_t addr, uint8_t data)
{
	//NLOGD("Mos6581", "Write(%#x, %#x)", addr, data);

	uint8_t reg = addr - Mos6581::REGISTER_BASE;

	prevRegValue = mRegs[reg];
	mRegs[reg] = data;

	if (reg >= Mos6581::R_VOICE1_FREQ_LO && reg <= Mos6581::R_VOICE1_SR) {
		//NLOGD("Mos6581", "Voice1 Write(%#x, %#x)", addr, data);
		mChannels[0].Write(addr, data);

	} else if (reg >= Mos6581::R_VOICE2_FREQ_LO && reg <= Mos6581::R_VOICE2_SR) {
		//NLOGD("Mos6581", "Voice2 Write(%#x, %#x)", addr, data);
		mChannels[1].Write(addr, data);

	} else if (reg >= Mos6581::R_VOICE3_FREQ_LO && reg <= Mos6581::R_VOICE3_SR) {
		//NLOGD("Mos6581", "Voice3 Write(%#x, %#x)", addr, data);
		mChannels[2].Write(addr, data);

	} else if (Mos6581::R_FILTER_FC_LO == reg) {

	} else if (Mos6581::R_FILTER_FC_HI == reg) {

	} else if (Mos6581::R_FILTER_RESFIL == reg) {

	} else if (Mos6581::R_FILTER_MODEVOL == reg) {

	}
}
