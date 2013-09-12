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
#include "YM2413.h"

// Patches taken from the MAME YM2413 emulator
const YM2413Instrument YM2413::MELODY_PATCHES[16] = {
	{0x49, 0x4c, 0x4c, 0x12, 0x00, 0x00, 0x00, 0x00},
    {0x61, 0x61, 0x1e, 0x17, 0xf0, 0x78, 0x00, 0x17},
    {0x13, 0x41, 0x1e, 0x0d, 0xd7, 0xf7, 0x13, 0x13},
    {0x13, 0x01, 0x99, 0x04, 0xf2, 0xf4, 0x11, 0x23},
    {0x21, 0x61, 0x1b, 0x07, 0xaf, 0x64, 0x40, 0x27},
    {0x22, 0x21, 0x1e, 0x06, 0xf0, 0x75, 0x08, 0x18},
    {0x31, 0x22, 0x16, 0x05, 0x90, 0x71, 0x00, 0x13},
    {0x21, 0x61, 0x1d, 0x07, 0x82, 0x80, 0x10, 0x17},
    {0x23, 0x21, 0x2d, 0x16, 0xc0, 0x70, 0x07, 0x07},
    {0x61, 0x61, 0x1b, 0x06, 0x64, 0x65, 0x10, 0x17},
    {0x61, 0x61, 0x0c, 0x18, 0x85, 0xf0, 0x70, 0x07},
    {0x23, 0x01, 0x07, 0x11, 0xf0, 0xa4, 0x00, 0x22},
    {0x97, 0xc1, 0x24, 0x07, 0xff, 0xf8, 0x22, 0x12},
    {0x61, 0x10, 0x0c, 0x05, 0xf2, 0xf4, 0x40, 0x44},
    {0x01, 0x01, 0x55, 0x03, 0xf3, 0x92, 0xf3, 0xf3},
    {0x61, 0x41, 0x89, 0x03, 0xf1, 0xf4, 0xf0, 0x13},
};

const YM2413Instrument YM2413::RHYTHM_PATCHES[3] = {
    {0x01, 0x01, 0x16, 0x00, 0xfd, 0xf8, 0x2f, 0x6d},	// Bass drumm
    {0x01, 0x01, 0x00, 0x00, 0xd8, 0xd8, 0xf9, 0xf8},	// Hihat/snare
    {0x05, 0x01, 0x00, 0x00, 0xf8, 0xba, 0x49, 0x55},	// Tom/cymbal
};

void YM2413EnvelopeGenerator::Reset()
{
	mPhase = ATTACK;
	mClocked = false;
	mOut = 0;
	mPos = 0;
}

void YM2413EnvelopeGenerator::Step()
{
	// ToDo: implement
}


void YM2413Channel::Reset()
{
	mMode = MODE_MELODY;
	mInstrument = YM2413::INSTRUMENT_CUSTOM;
	LoadPatch(mInstrument);

	mOctave = 1;
	mFNumber = 0;
}

void YM2413Channel::Step()
{
	// ToDo: implement
}

void YM2413Channel::LoadPatch(uint8_t patchNum)
{
	// ToDo: implement
}

void YM2413Channel::Write(uint32_t addr, uint8_t data)
{
	if (addr >= YM2413::R_CHN0_FREQ_LO && addr <= YM2413::R_CHN8_FREQ_LO) {
		mFNumber = (mFNumber & 0x100) | data;

	} else if (addr >= YM2413::R_CHN0_KEY_FREQ_HI && addr <= YM2413::R_CHN8_KEY_FREQ_HI) {
		// ToDo: handle
		mFNumber = (mFNumber & 0xFF) | ((uint16_t)(data & 1) << 8);
		mOctave = (data >> 1) & 7;

	} else if (addr >= YM2413::R_CHN0_INSTR_VOL && addr <= YM2413::R_CHN8_INSTR_VOL) {
		mVol = data & 0x0F;
		mInstrument = data >> 4;
		LoadPatch(mInstrument);
	}
}


void YM2413::Reset()
{
	// ToDo: implement
	for (int i = 0; i < 9; i++) {
		mChannels[i].Reset();
	}
	mEG.Reset();
}

void YM2413::Step()
{
	mEG.Step();
	for (int i = 0; i < 9; i++) {
		mChannels[i].Step();
	}
}

void YM2413::Write(uint32_t addr, uint8_t data)
{
	if (addr >= R_CHN0_FREQ_LO
			&& addr <= R_CHN8_INSTR_VOL
			&& (addr & 0x0F) < 9) {
		mChannels[addr & 0x0F].Write(addr, data);
	}
}

