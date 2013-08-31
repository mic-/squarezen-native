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
#include "SDsp.h"


void SDspEnvelopeGenerator::Reset()
{
	// ToDo: implement
	mSetting = GAIN_SETTING_DIRECT;
	mMode = GAIN_MODE_LINEAR_DEC;
}


void SDspEnvelopeGenerator::Step()
{
	// ToDo: implement
}


void SDspVoice::Reset()
{
	// ToDo: implement
	mEG.Reset();
}


void SDspVoice::Step()
{
	// ToDo: impelement
}


void SDspVoice::Write(uint32_t addr, uint8_t val)
{
	uint8_t group = addr & 0x0F;

	switch (group) {
	case SDsp::R_VxPITCHL:
		break;
	case SDsp::R_VxPITCHH:
		break;
	case SDsp::R_VxSRCN:
		break;
	case SDsp::R_VxADSR1:
		break;
	case SDsp::R_VxADSR2:
		break;
	case SDsp::R_VxGAIN:
		break;
	}
}

void SDsp::Step()
{
	for (int i = 0; i < 8; i++) {
		mVoices[i].Step();
	}
}


void SDsp::Write(uint32_t addr, uint8_t data)
{
	NLOGD("SDsp", "Write(%#x, %#x)", addr, data);

	uint8_t group = addr & 0x0F;
	uint8_t chn = (addr >> 4) & 7;

	switch (group) {
	case R_VxPITCHL:
	case R_VxPITCHH:
	case R_VxSRCN:
	case R_VxADSR1:
	case R_VxADSR2:
	case R_VxGAIN:
		mVoices[chn].Write(addr, data);
		break;
	}
}