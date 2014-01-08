/*
 * AyPlayer.h
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

#ifndef AYPLAYER_H_
#define AYPLAYER_H_

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include "EmuCommon.h"
#include "YM2149.h"
#include "MusicPlayer.h"
#include "Z80.h"


class AyPlayer : public MusicPlayer {
public:
	AyPlayer();
	virtual ~AyPlayer();

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	virtual size_t GetNumChannels() const { return 3; }
	virtual void GetChannelOutputs(int16_t *outputs) const;

	typedef struct __attribute__ ((__packed__))
	{
		char ID[4];					// "ZXAY"
		char typeID[4];				// "EMUL"
		uint8_t fileVersion;
		uint8_t playerVersion;
		uint16_t specialPlayer;		// only used by one song (can be ignored?)
		uint16_t authorOffset;
		uint16_t miscOffset;
		uint8_t numSongs;
		uint8_t firstSong;
		uint16_t songsStructOffset;
	} AyFileHeader;

	typedef struct __attribute__ ((__packed__))
	{
		uint16_t songNameOffset;
		uint16_t songDataOffset;
	} AySongStruct;

	typedef struct __attribute__ ((__packed__))
	{
		// Specifies which Paula channels will be used to play which YM channels
		// when emulating on an Amiga.
		uint8_t aChan;
		uint8_t bChan;
		uint8_t cChan;
		uint8_t nChan;			// noise

		uint16_t songLength;	// in 1/50 s
		uint16_t fadeLength;	// in 1/50 s
		uint8_t hiReg;
		uint8_t loReg;
		uint16_t pointsOffset;
		uint16_t blocksOffset;
	} AyEmulSongData;

private:

	Z80 		 *mZ80;
	YmChip 		 mAy;
	AyFileHeader mFileHeader;
	std::vector<AySongStruct*> mSongs;
};

MusicPlayer *AyPlayerFactory();

#endif /* AYPLAYER_H_ */
