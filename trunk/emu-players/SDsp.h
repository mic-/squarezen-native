/*
 * SDsp.h
 *
 *  Created on: Aug 14, 2013
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

#ifndef SDSP_H_
#define SDSP_H_

#include <stdint.h>
#include "Oscillator.h"


class SDspEnvelopeGenerator : public Oscillator
{
public:
	virtual ~SDspEnvelopeGenerator() {}

	virtual void Reset();
	virtual void Step();

	enum
	{
		GAIN_SETTING_ADSR,
		GAIN_SETTING_DIRECT,
		GAIN_SETTING_CUSTOM,
	};

	// Custom gain modes
	enum
	{
		GAIN_MODE_LINEAR_DEC = 0,
		GAIN_MODE_EXP_DEC = 1,
		GAIN_MODE_LINEAR_INC = 2,
		GAIN_MODE_BENT_INC = 3,
	};

private:
	uint8_t mSetting, mMode;

};


class SDspVoice : public Oscillator
{
public:
	virtual ~SDspVoice() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	SDspEnvelopeGenerator mEG;
	uint32_t mBrrShift;
	uint32_t mBrrFilter;
};


class SDsp
{
public:
	void Write(uint32_t addr, uint8_t data);
	void Step();

	enum {
		S_DSP_CLOCK = 2048000,
		S_DSP_CLOCK_DIVIDER = 1,	// Used for controlling the speed at which the S-DSP is emulated
	};

	// Registers
	enum {
		R_VxPITCHL = 0x02,
		R_V0PITCHL = 0x02,
		R_V1PITCHL = 0x12,
		R_V2PITCHL = 0x22,
		R_V3PITCHL = 0x32,
		R_V4PITCHL = 0x42,
		R_V5PITCHL = 0x52,
		R_V6PITCHL = 0x62,
		R_V7PITCHL = 0x72,

		R_VxPITCHH = 0x03,
		R_V0PITCHH = 0x03,
		R_V1PITCHH = 0x13,
		R_V2PITCHH = 0x23,
		R_V3PITCHH = 0x33,
		R_V4PITCHH = 0x43,
		R_V5PITCHH = 0x53,
		R_V6PITCHH = 0x63,
		R_V7PITCHH = 0x73,

		R_VxSRCN = 0x04,
		R_V0SRCN = 0x04,
		R_V1SRCN = 0x14,
		R_V2SRCN = 0x24,
		R_V3SRCN = 0x34,
		R_V4SRCN = 0x44,
		R_V5SRCN = 0x54,
		R_V6SRCN = 0x64,
		R_V7SRCN = 0x74,

		R_VxADSR1 = 0x05,
		R_V0ADSR1 = 0x05,
		R_V1ADSR1 = 0x15,
		R_V2ADSR1 = 0x25,
		R_V3ADSR1 = 0x35,
		R_V4ADSR1 = 0x45,
		R_V5ADSR1 = 0x55,
		R_V6ADSR1 = 0x65,
		R_V7ADSR1 = 0x75,

		R_VxADSR2 = 0x06,
		R_V0ADSR2 = 0x06,
		R_V1ADSR2 = 0x16,
		R_V2ADSR2 = 0x26,
		R_V3ADSR2 = 0x36,
		R_V4ADSR2 = 0x46,
		R_V5ADSR2 = 0x56,
		R_V6ADSR2 = 0x66,
		R_V7ADSR2 = 0x76,

		R_VxGAIN = 0x07,
		R_V0GAIN = 0x07,
		R_V1GAIN = 0x17,
		R_V2GAIN = 0x27,
		R_V3GAIN = 0x37,
		R_V4GAIN = 0x47,
		R_V5GAIN = 0x57,
		R_V6GAIN = 0x67,
		R_V7GAIN = 0x77,

		R_VxENVX = 0x08,
		R_VxOUTX = 0x09,
		R_FIRx = 0x0F,

		R_MOVLL = 0x0C,
		R_EFB = 0x0D,	// Echo feedback volume (-128..+127)
		R_MOVLR = 0x1C,
		R_EVOLL = 0x2C,
		R_PMON = 0x2D,
		R_EVOLR = 0x3C,
		R_NON = 0x3D,
		R_KON = 0x4C,
		R_EON = 0x4D,
		R_KOFF = 0x5C,
		R_DIR = 0x5D,	// Sample directory address (/ 0x100)
		R_FLG = 0x6C,
		R_ESA = 0x6D,	// Echo buffer start address (/ 0x100)
		R_ENDX = 0x7C,
		R_EDL = 0x7D, 	// Echo delay (d0..3: size in 2kB (==16ms) steps)
	};

private:
	SDspVoice mVoices[8];
};

#endif /* SDSP_H_ */
