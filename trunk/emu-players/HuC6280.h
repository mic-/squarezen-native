/*
 * HuC6280.h
 *
 *  Created on: Aug 30, 2013
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

#ifndef HUC62802_H_
#define HUC62802_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "Oscillator.h"

class HuC6280Mapper;

class HuC6280PsgChannel : public Oscillator
{
public:
	HuC6280PsgChannel();
	virtual ~HuC6280PsgChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t data);

	int16_t mOut;
	uint16_t mVolL, mVolR;
	uint16_t mEnable;
	uint8_t mWaveformRam[32];
	uint16_t mWaveReadPos, mWaveWritePos;
};


class HuC6280Psg
{
public:
	HuC6280Psg();
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	enum {
		R_CHN_SELECT = 0x800,
		R_BALANCE = 0x801,
		R_FREQ_LO = 0x802,
		R_FREQ_HI = 0x803,
		R_ENABLE = 0x804,
		R_CHN_BALANCE = 0x805,
		R_WAVE_DATA = 0x806,
		R_NOISE = 0x807,
		R_LFO_FREQ = 0x808,
		R_LFO_CTRL = 0x809,
	};

	// For R_ENABLE
	enum {
		WRITE_MODE = 0xC0,
		WRITE_WAVEFORM_RAM = 0x00,
		CHN_ENABLE = 0x80,
		DDA_ENABLE = 0x40,
	};

	HuC6280PsgChannel mChannels[6];
	uint16_t mMasterVolL, mMasterVolR;
	uint16_t mChannelSelect;
};


class HuC6280
{
public:
	void Reset();
	void Run(uint32_t maxCycles);
	void SetMapper(HuC6280Mapper *mapper) { mMemory = mapper; }

	enum {
		FLAG_C = 0x01,
		FLAG_Z = 0x02,
		FLAG_I = 0x04,
		FLAG_D = 0x08,
		FLAG_T = 0x10,
		FLAG_V = 0x40,
		FLAG_N = 0x80,
	};

	enum {
		R_TIMER_COUNT = 0xC00,
		R_TIMER_CTRL = 0xC01,
	};

	struct {
		uint8_t A, X, Y, S, F;
		uint16_t PC;
	} mRegs;

	uint32_t mCycles;
	uint8_t mSpeed;
	uint8_t mMPR[8];
	uint8_t mMprLatch;
private:
	HuC6280Mapper *mMemory;
};

#endif

