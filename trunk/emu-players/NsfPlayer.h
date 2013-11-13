/*
 * NsfPlayer.h
 *
 *  Created on: Jun 2, 2013
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

#ifndef NSFPLAYER_H_
#define NSFPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "Emu6502.h"
#include "Emu2A03.h"
#include "KonamiVrc6.h"
#include "Namco163.h"
#include "Sunsoft5B.h"
#include "NsfMapper.h"


class NsfPlayer : public MusicPlayer
{
public:
	NsfPlayer();
	virtual ~NsfPlayer();

	typedef struct __attribute__ ((__packed__))
	{
		char ID[5];
		uint8_t version;
		uint8_t numSongs;
		uint8_t firstSong;		// 1-based
		uint16_t loadAddress;
		uint16_t initAddress;
		uint16_t playAddress;
		char title[32];			// ASCIIZ
		char author[32];		// ...
		char copyright[32];		// ...
		uint16_t ntscSpeedUs;
		uint8_t initialBanks[8];
		uint16_t palSpeedUs;
		uint8_t region;
		uint8_t extraChips;
		uint32_t reserved;
	} NsfFileHeader;

	enum {
		REGION_NTSC = 0x00,
		REGION_PAL = 0x01,
		REGION_DUAL = 0x02,
	};

	enum {
		USES_VRC6 = 0x01,
		USES_VRC7 = 0x02,
		USES_FDS = 0x04,
		USES_MMC5 = 0x08,
		USES_N163 = 0x10,
		USES_SUNSOFT_5B = 0x20,
	};

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	virtual size_t GetNumChannels() const;
	virtual void GetChannelOutputs(int16_t *outputs) const;

	virtual void SetSubSong(uint32_t subSong);

private:
	void Execute6502(uint16_t address);

	Emu6502 *m6502;
	Emu2A03 *m2A03;
	KonamiVrc6 *mVrc6;
	Namco163 *mN163;
	Sunsoft5B *mSunsoft5B;
	NsfMapper *mMemory;

	uint32_t mVrc6Synth, mN163Synth, mSunsoft5BSynth;

	NsfFileHeader mFileHeader;
	bool mSongIsBankswitched;
	uint32_t mFrameCycles, mCycleCount, mPlayCounter;
};

MusicPlayer *NsfPlayerFactory();

#endif /* NSFPLAYER_H_ */
