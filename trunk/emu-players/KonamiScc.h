/*
 * KonamiScc.h
 *
 *  Created on: Aug 29, 2013
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

#ifndef KONAMISCC_H_
#define KONAMISCC_H_

#include <stdint.h>
#include "Oscillator.h"


class KonamiSccChannel : public Oscillator
{
public:
	virtual ~KonamiSccChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t data);

	uint8_t *mWaveform;
	uint16_t mVol;
	uint16_t mOut;
	bool mEnabled;
};


class KonamiScc
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	enum {
		R_WAVEFORM_RAM = 0x9800,
		R_CHN1_PERL = 0x9880,
		R_CHN1_PERH = 0x9881,
		R_CHN2_PERL = 0x9882,
		R_CHN2_PERH = 0x9883,
		R_CHN3_PERL = 0x9884,
		R_CHN3_PERH = 0x9885,
		R_CHN4_PERL = 0x9886,
		R_CHN4_PERH = 0x9887,
		R_CHN5_PERL = 0x9888,
		R_CHN5_PERH = 0x9889,
		R_CHN1_VOL = 0x988A,
		R_CHN2_VOL = 0x988B,
		R_CHN3_VOL = 0x988C,
		R_CHN4_VOL = 0x988D,
		R_CHN5_VOL = 0x988E,
		R_CHN_ENABLE = 0x988F,
	};

	KonamiSccChannel mChannels[5];
private:
	uint8_t mWaveformRam[128];
};

#endif
