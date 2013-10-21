/*
 * YM2413.h
 *
 *  Created on: Sep 12, 2013
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

#ifndef YM2413_H_
#define YM2413_H_

#include "Oscillator.h"

class YM2413Channel;

typedef struct {
	uint8_t modulatorModulation;
	uint8_t carrierModulation;
	uint8_t levels;
	uint8_t waveformFeedback;
	uint8_t modulatorAD;
	uint8_t carrierAD;
	uint8_t modulatorSR;
	uint8_t carrierSR;
} YM2413Instrument;


class YM2413EnvelopeGenerator : public Oscillator
{
public:
	virtual ~YM2413EnvelopeGenerator() {}

	virtual void Reset();
	virtual void Step();

	void SetChannel(YM2413Channel *channel) { mChannel = channel; }

	// Envelope phases
	typedef enum
	{
		ATTACK = 0,
		DECAY,
		SUSTAIN,
		RELEASE,
	} Phase;

	YM2413Channel *mChannel;
	uint8_t mOut;
	uint16_t mSustainLevel;
	Phase mPhase;
	bool mClocked;
};


class YM2413Channel : public Oscillator
{
public:
	virtual ~YM2413Channel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t data);

	void LoadPatch(uint8_t patchNum);

	enum {
		MODE_MELODY,
		MODE_RHYTHM,
	};

	YM2413EnvelopeGenerator mEG;
	uint16_t mFNumber;
	uint8_t mKeyScaling;
	uint8_t mMultiplier;
	uint8_t mSustain;
	uint8_t mOctave;
	uint8_t mInstrument;
	uint8_t mVol;
	uint8_t mMode;
};


class YM2413
{
public:
	void Reset();
	void Step();

	/**
	 * @param addr address to write to (0 = address port, 1 = data port)
	 * @param data data to write to the port
	 */
	void Write(uint32_t addr, uint8_t data);

	// Registers
	enum {
		R_CHN0_FREQ_LO = 0x10,
		R_CHN1_FREQ_LO = 0x11,
		R_CHN2_FREQ_LO = 0x12,
		R_CHN3_FREQ_LO = 0x13,
		R_CHN4_FREQ_LO = 0x14,
		R_CHN5_FREQ_LO = 0x15,
		R_CHN6_FREQ_LO = 0x16,
		R_CHN7_FREQ_LO = 0x17,
		R_CHN8_FREQ_LO = 0x18,

		R_CHN0_KEY_FREQ_HI = 0x20,
		R_CHN1_KEY_FREQ_HI = 0x21,
		R_CHN2_KEY_FREQ_HI = 0x22,
		R_CHN3_KEY_FREQ_HI = 0x23,
		R_CHN4_KEY_FREQ_HI = 0x24,
		R_CHN5_KEY_FREQ_HI = 0x25,
		R_CHN6_KEY_FREQ_HI = 0x26,
		R_CHN7_KEY_FREQ_HI = 0x27,
		R_CHN8_KEY_FREQ_HI = 0x28,

		R_CHN0_INSTR_VOL = 0x30,
		R_CHN1_INSTR_VOL = 0x31,
		R_CHN2_INSTR_VOL = 0x32,
		R_CHN3_INSTR_VOL = 0x33,
		R_CHN4_INSTR_VOL = 0x34,
		R_CHN5_INSTR_VOL = 0x35,
		R_CHN6_INSTR_VOL = 0x36,
		R_CHN7_INSTR_VOL = 0x37,
		R_CHN8_INSTR_VOL = 0x38,
	};

	// Instruments
	enum {
		INSTRUMENT_CUSTOM = 0,
		INSTRUMENT_VIOLIN = 1,
		INSTRUMENT_GUITAR = 2,
		INSTRUMENT_PIANO = 3,
		INSTRUMENT_FLUTE = 4,
		INSTRUMENT_CLARINET = 5,
		INSTRUMENT_OBOE = 6,
		INSTRUMENT_TRUMPET = 7,
		INSTRUMENT_ORGAN = 8,
		INSTRUMENT_HORN = 9,
		INSTRUMENT_SYNTHESIZER = 10,
		INSTRUMENT_HARPSICHORD = 11,
		INSTRUMENT_VIBRAPHONE = 12,
		INSTRUMENT_SYNTHESIZER_BASS = 13,
		INSTRUMENT_ACOUSTIC_BASS = 14,
		INSTRUMENT_ELECTRIC_GUITAR = 15,
	};

	static const YM2413Instrument MELODY_PATCHES[16];
	static const YM2413Instrument RHYTHM_PATCHES[3];

	YM2413Channel mChannels[9];
	uint8_t	mAddressLatch;
};

#endif	/* YM2413_H_ */
