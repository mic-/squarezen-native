/*
 * YmPlayer.h
 *
 *  Created on: May 24, 2013
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

#ifndef YMPLAYER_H_
#define YMPLAYER_H_

#include <stdint.h>
#include <stddef.h>
#include "YM2149.h"
#include "Blip_Buffer.h"
#include "MusicPlayer.h"


class YmPlayer : public MusicPlayer {
public:
	YmPlayer();

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	virtual size_t GetNumChannels() const { return 3; }
	virtual void GetChannelOutputs(int16_t *outputs) const;

	typedef struct __attribute__ ((__packed__))
	{
		uint8_t archiveHeaderSize;
		uint8_t headerChecksum;
		char methodId[5];
		uint32_t compressedSize;
		uint32_t uncompressedSize;
		uint32_t originalTimestamp;
		uint8_t fileAttribute;
		uint8_t level;
		uint8_t filenameLength;
	} LhFileHeader;

	YmChip mChip;
	LhFileHeader *mLhHeader;
	uint32_t mLhDataPos,mLhDataAvail;
	uint8_t *mYmData;
private:

	uint32_t mCycleCount, mFrameCycles;
	uint32_t mFrame, mNumFrames;
	uint32_t mDataOffs;
	uint8_t *mYmRegStream;
	uint8_t mYmRegs[16];

	int16_t *mTempBuffer;
};

MusicPlayer *YmPlayerFactory();

#endif /* YMPLAYER_H_ */
