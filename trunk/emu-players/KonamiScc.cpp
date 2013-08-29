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

#include "NativeLogger.h"
#include "KonamiScc.h"

void KonamiSccChannel::Reset()
{
	// ToDo: implement
	mOut = 0;
}


void KonamiSccChannel::Step()
{
	mPos++;

	if (mPos >= mPeriod) {
		mPos -= mPeriod;
		mOut = mWaveform[mStep];
		mStep = (mStep + 1) & 0x1F;
	}
}


void KonamiSccChannel::Write(uint32_t addr, uint8_t data)
{
	if (addr >= KonamiScc::R_CHN1_PERL && addr <= KonamiScc::R_CHN5_PERH) {
		if (addr & 1) {
			mPeriod = (mPeriod & 0xFF) | ((uint16_t)(data & 0xF) << 8);
		} else {
			mPeriod = (mPeriod & 0xF00) | data;
		}
	} else if (addr >= KonamiScc::R_CHN1_VOL && addr <= KonamiScc::R_CHN5_VOL) {
		mVol = data;
	}
}


void KonamiScc::Reset()
{
	mChannels[0].mWaveform = &mWaveformRam[0x00];
	mChannels[1].mWaveform = &mWaveformRam[0x20];
	mChannels[2].mWaveform = &mWaveformRam[0x40];
	mChannels[3].mWaveform = mChannels[4].mWaveform = &mWaveformRam[0x60];
}


void KonamiScc::Step()
{
	for (int i = 0; i < 5; i++) {
		mChannels[i].Step();
	}
}


void KonamiScc::Write(uint32_t addr, uint8_t data)
{
	if (addr >= R_WAVEFORM_RAM && addr < R_CHN1_PERL) {
		mWaveformRam[addr & 0x7F] = data;
	} else if (addr >= 0x9880 && addr <= 0x989F) {
		addr &= 0x988F;
		if (addr >= R_CHN1_PERL && addr <= R_CHN5_PERH) {
			mChannels[(addr - R_CHN1_PERL) / 2].Write(addr, data);
		} else if (addr >= R_CHN1_VOL && addr <= R_CHN5_VOL) {
			mChannels[addr - R_CHN1_VOL].Write(addr, data);
		} else if (addr == R_CHN_ENABLE) {
		}
	}
}
