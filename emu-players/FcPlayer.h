/*
 * FcPlayer.h
 *
 *  Created on: Oct 24, 2013
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

#ifndef FCPLAYER_H_
#define FCPLAYER_H_

#include <string>
#include <vector>
#include <stdint.h>
#include "MusicPlayer.h"


class FcPlayer : public MusicPlayer
{
public:
	FcPlayer();
	virtual ~FcPlayer();

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	virtual size_t GetNumChannels() const { return 4; }
	virtual void GetChannelOutputs(int16_t *outputs) const;

	typedef struct __attribute__ ((__packed__))
	{
		uint16_t length;		// in words
		uint16_t loopStart;		// in bytes
		uint16_t loopLength;	// in words (1 == no loop)
	} FcSampleInfo;

	typedef struct __attribute__ ((__packed__))
	{
		uint8_t pattern;
		uint8_t transpose;
		uint8_t soundTranspose;
	} FcSequenceVoice;

	typedef struct __attribute__ ((__packed__))
	{
		FcSequenceVoice voices[4];
		uint8_t defaultSpeed;
	} FcSequence;

	typedef struct __attribute__ ((__packed__))
	{
		char ID[4];		// "SMOD"
		uint32_t sequenceLength;	// in bytes
		uint32_t patternOffset;
		uint32_t patternLength;
		uint32_t freqSequenceOffset;
		uint32_t freqSequenceLength;
		uint32_t volSequenceOffset;
		uint32_t volSequenceLength;
		uint32_t sampleDataOffset;
		uint32_t sampleDataLength;
		FcSampleInfo sampleInfo[10];
	}  Fc13FileHeader;

	typedef struct __attribute__ ((__packed__))
	{
		char ID[4];		// "FC14"
		uint32_t sequenceLength;	// in bytes
		uint32_t patternOffset;
		uint32_t patternLength;
		uint32_t freqSequenceOffset;
		uint32_t freqSequenceLength;
		uint32_t volSequenceOffset;
		uint32_t volSequenceLength;
		uint32_t sampleDataOffset;
		uint32_t waveTableOffset;
		FcSampleInfo sampleInfo[10];
		uint8_t waveTableLengths[80];	// in words
	}  Fc14FileHeader;

	typedef struct __attribute__ ((__packed__))
	{
		uint8_t note;
		uint8_t infoInstrument;
	} FcPatternRow;

	typedef struct __attribute__ ((__packed__))
	{
		FcPatternRow rows[32];
	} FcPattern;


	// Frequence sequence commands
	enum
	{
		FREQ_CMD_JUMP = 0xE0,			// $E0 x	Position jump
		FREQ_CMD_END = 0xE1,			// $E1		End of sequence
		FREQ_CMD_SET_WAVEFORM = 0xE2,	// $E2 x	Set waveform (will trig it)
		FREQ_CMD_VIBRATO = 0xE3,		// $E3 x y	New vibrato
		FREQ_CMD_CHG_WAVEFORM = 0xE4,	// $E4 x	Change waveform (waveform doesn't change until the current one loops)
		FREQ_CMD_PATTERN_JUMP = 0xE7,	// $E7 x	Pattern jump
		FREQ_CMD_SET_SUSTAIN = 0xE8,	// $E8 x	Set sustain time
		FREQ_CMD_SET_SAMPLE = 0xE9,		// $E9 x y	Set sample (x = instrument, y = sample in the instrument)
		FREQ_CMD_PITCHBEND = 0xEA,		// $EA x y	Pitchbend
	};

	// Volume sequence commands
	enum
	{
		VOL_CMD_JUMP = 0xE0,			// $E0 x	Position jump
		VOL_CMD_END = 0xE1,				// $E1		End of sequence
		VOL_CMD_SET_SUSTAIN = 0xE8,		// $E8 x	Set volume sustain
		VOL_CMD_VOLBEND = 0xEA,			// $EA x y	Volume bend
	};

private:
	bool mIsFc14;
	void *mFileHeader;
	std::vector<FcSequence> *mSequences;
	std::vector<FcPattern> *mPatterns;
	uint32_t mCurrSequence;
	static const std::string FC13_ID;
	static const std::string FC14_ID;
};

MusicPlayer *FcPlayerFactory();

#endif /* GBSPLAYER_H_ */





