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
}


void KonamiSccChannel::Step()
{
	// ToDo: implement
}


void KonamiScc::Reset()
{
	mChannels[0].mWaveform = &mWaveformRam[0];
	mChannels[1].mWaveform = &mWaveformRam[32];
	mChannels[2].mWaveform = &mWaveformRam[64];
	mChannels[3].mWaveform = mChannels[4].mWaveform = &mWaveformRam[96];
}


void KonamiScc::Step()
{
	for (int i = 0; i < 5; i++) {
		mChannels[i].Step();
	}
}


void KonamiScc::Write(uint32_t addr, uint8_t data)
{
	if (addr >= 0x9800 && addr < 0x9880) {
		mWaveformRam[addr & 0x7F] = data;
	}
}
