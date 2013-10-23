/*
 * GbsPlayer.h
 *
 *  Created on: May 29, 2013
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

#ifndef GBSPLAYER_H_
#define GBSPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "GbPapu.h"


class GbsPlayer : public MusicPlayer
{
public:
	GbsPlayer();
	virtual ~GbsPlayer();

	typedef struct __attribute__ ((__packed__))
	{
		char ID[3];
		uint8_t version;
		uint8_t numSongs;
		uint8_t firstSong;
		uint16_t loadAddress;
		uint16_t initAddress;
		uint16_t playAddress;
		uint16_t SP;
		uint8_t timerMod;
		uint8_t timerCtrl;
		char title[32];
		char author[32];
		char copyright[32];
	}  GbsFileHeader;

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	void SetMasterVolume(int left, int right);
	uint16_t GetLoadAddress() const { return mFileHeader.loadAddress; }

	virtual size_t GetNumChannels() const { return 4; }
	virtual void GetChannelOutputs(int16_t *outputs) const;

	virtual void SetSubSong(uint32_t subSong);

private:
	void PresentBuffer(int16_t *out, Blip_Buffer *inL, Blip_Buffer *inR);
	void ExecuteGbZ80(uint16_t address);

	Blip_Buffer *mBlipBufRight;
	Blip_Synth<blip_low_quality,4096> *mSynthRight;
	GbsFileHeader mFileHeader;
	GbPapuChip mPapu;
	uint32_t mCycleCount, mFrameCycles;
};

MusicPlayer *GbsPlayerFactory();

#endif /* GBSPLAYER_H_ */
