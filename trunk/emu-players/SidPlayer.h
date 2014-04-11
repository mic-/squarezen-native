/*
 * SidPlayer.h
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

#ifndef SIDPLAYER_H_
#define SIDPLAYER_H_

#include <memory>
#include <string>
#include <stdint.h>
#include "Emu6502.h"
#include "Mos6581.h"
#include "SidMapper.h"
#include "MusicPlayer.h"


class SidPlayer : public MusicPlayer
{
public:
	SidPlayer();
	virtual ~SidPlayer();

	typedef struct __attribute__ ((__packed__))
	{
		char magic[4];
		uint16_t version;
		uint16_t dataOffset;
		uint16_t loadAddress;
		uint16_t initAddress;
		uint16_t playAddress;
		uint16_t numSongs;
		uint16_t firstSong;	// 1-based
		uint32_t speed;
		char title[32];		// ASCIIZ
		char author[32];	// ...
		char copyright[32];	// ...
		// END of v1 header
		uint16_t flags;
		uint8_t startPage;
		uint8_t pageLength;
		uint16_t reserved;
		// END of v2 header
	} PsidFileHeader;

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	virtual size_t GetNumChannels() const { return 3; }
	virtual void GetChannelOutputs(int16_t *outputs) const;

	void TimerIrq(uint8_t timerNum);

	virtual void SetSubSong(uint32_t subSong);
	void SetMasterVolume(int masterVol);

private:
	void Execute6502(uint16_t address);

	Emu6502 *m6502;
	Mos6581 *mSid;
	std::shared_ptr<SidMapper> mMemory;
	uint32_t mFrameCycles, mCycleCount;
	PsidFileHeader mFileHeader;
	uint16_t mDriverPage;
};

MusicPlayer *SidPlayerFactory();

#endif /* SIDPLAYER_H_ */
