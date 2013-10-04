/*
 * SgcPlayer.h
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

#ifndef SGCPLAYER_H_
#define SGCPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "Z80.h"
#include "SN76489.h"
#include "SgcMapper.h"


class SgcPlayer : public MusicPlayer
{
public:
	SgcPlayer();
	virtual ~SgcPlayer();

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	enum {
		SYSTEM_SMS = 0,
		SYSTEM_GG = 1,
		SYSTEM_CV = 2,
	};

	enum {
		REGION_NTSC = 0,
		REGION_PAL = 1,
	};

	typedef struct __attribute__ ((__packed__))
	{
		char signature[4];
		uint8_t version;
		uint8_t region;
		uint8_t scanlinesPerIrq;
		uint8_t reserved1;
		uint16_t loadAddress;
		uint16_t initAddress;
		uint16_t playAddress;
		uint16_t stackPointer;
		uint16_t reserved2;
		uint16_t rstAddress[7];
		uint8_t mapperInit[4];
		uint8_t firstSong;
		uint8_t numSongs;
		uint8_t firstSoundEffect;
		uint8_t lastSoundEffect;
		uint8_t systemType;
		uint8_t reserved3[7];
		char title[32];
		char artist[32];
		char copyright[32];
	} SgcFileHeader;

private:
	SgcFileHeader mFileHeader;
	Z80 *mZ80;
	SnChip *mSN76489;
	SgcMapper *mMemory;
};

MusicPlayer *SgcPlayerFactory();

#endif	/* SGCPLAYER_H_ */

