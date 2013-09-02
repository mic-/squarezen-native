/*
 * KssPlayer.h
 *
 *  Created on: Sep 2, 2013
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

#ifndef KSSPLAYER_H_
#define KSSPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "Z80.h"
#include "KssMapper.h"
#include "SN76489.h"
#include "YM2149.h"
#include "KonamiScc.h"


class KssPlayer : public MusicPlayer
{
public:
	KssPlayer();
	virtual ~KssPlayer();

	virtual int Prepare(std::string fileName);
	virtual int Run(uint32_t numSamples, int16_t *buffer);
	virtual int Reset();

	void SetSccEnabled(bool enable);

	typedef struct __attribute__ ((__packed__))
	{
		char ID[4];
		uint16_t loadAddress;
		uint16_t initDataSize;
		uint16_t initAddress;
		uint16_t playAddress;
		uint8_t firstBank;
		uint8_t extraBanks;
		uint8_t reserved;
		uint8_t extraChips;
	} KssFileHeader;

	enum {
		FMPAC_MASK     = 0x03,
		FMUNIT_MASK    = 0x03,
		SN76489_MASK   = 0x03,
		RAM1_MASK      = 0x06,
		GG_STEREO_MASK = 0x06,
		MSX_AUDIO_MASK = 0x0A,
		RAM2_MASK      = 0x0A,

		USES_FMPAC     = 0x01,
		USES_FMUNIT    = 0x03,
		USES_SN76489   = 0x02,
		USES_RAM1      = 0x04,
		USES_GG_STEREO = 0x06,
		USES_MSX_AUDIO = 0x08,
		USES_RAM2      = 0x0A,
	};

private:
	void PresentBuffer(int16_t *out, Blip_Buffer *in);

	KssFileHeader mFileHeader;
	Z80 *mZ80;
	KssMapper *mMemory;
	YmChip *mAy;
	SnChip *mSN76489;
	KonamiScc *mScc;
	bool mSccEnabled;
};

#endif
