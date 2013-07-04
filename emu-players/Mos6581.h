/*
 * Mos6581.h
 *
 *  Created on: Jul 4, 2013
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

#ifndef MOS6581_H_
#define MOS6581_H_

#include <stdint.h>
#include "Oscillator.h"

class Mos6581
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	enum
	{
		REGISTER_BASE	  = 0xD400,

		R_VOICE1_FREQ_LO  = 0x00,	// 16 bits, Fout = nnnn*0.0596
		R_VOICE1_FREQ_HI  = 0x01,	// ...
		R_VOICE1_PW_LO    = 0x02,	// 12 bits, PWout = nnn/40.95 %
		R_VOICE1_PW_HI    = 0x03,	// ...
		R_VOICE1_CTRL     = 0x04,
		R_VOICE1_AD       = 0x05,  // Attack rates: 2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000 ms
		R_VOICE1_SR       = 0x06,  // DSR rates: 6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000 ms

		R_VOICE2_FREQ_LO  = 0x07,
		R_VOICE2_FREQ_HI  = 0x08,
		R_VOICE2_PW_LO    = 0x09,
		R_VOICE2_PW_HI    = 0x0A,
		R_VOICE2_CTRL     = 0x0B,
		R_VOICE2_AD       = 0x0C,
		R_VOICE2_SR       = 0x0D,

		R_VOICE3_FREQ_LO  = 0x0E,
		R_VOICE3_FREQ_HI  = 0x0F,
		R_VOICE3_PW_LO    = 0x10,
		R_VOICE3_PW_HI    = 0x11,
		R_VOICE3_CTRL     = 0x12,
		R_VOICE3_AD       = 0x13,
		R_VOICE3_SR       = 0x14,

		R_FILTER_FC_LO    = 0x15,	// -----lll
		R_FILTER_FC_HI    = 0x16,   // hhhhhhhh
		R_FILTER_RESFIL   = 0x17,
		R_FILTER_MODEVOL  = 0x18,
	};
};

#endif
