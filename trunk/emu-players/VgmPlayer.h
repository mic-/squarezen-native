/*
 * VgmPlayer.h
 *
 *  Created on: May 23, 2013
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

#ifndef VGMPLAYER_H_
#define VGMPLAYER_H_

#include "SN76489.h"
#include "Blip_Buffer.h"
#include "MusicPlayer.h"

class SnChip;


class VgmPlayer : public MusicPlayer
{
public:
	VgmPlayer();

	virtual int Prepare(std::string fileName);
	virtual int Run(uint32_t numSamples, int16_t *buffer);
	virtual int Reset();

	typedef struct __attribute__ ((__packed__))
	{
		char ID[4];
		uint32_t eofOffset;
		uint32_t version;
		uint32_t sn76489Clock;
		uint32_t ym2413Clock;
		uint32_t gd3Offset;
		uint32_t totalSamples;
		uint32_t loopOffset;
		uint32_t loopSamples;

		// Added in 1.01
		uint32_t rateHz;

		// Added in 1.10
		uint16_t lfsrTaps;
		uint8_t lfsrWidth;
		uint8_t sn76489Flags;
		uint32_t ym2612Clock;
		uint32_t ym2151Clock;

		// Added in 1.50,1.51
		uint32_t dataOffset;
		uint32_t segaPcmClock;
		uint32_t segaPcmItfReg;
	} VgmFileHeader;
	
private:
	void ParseGd3(size_t fileSize);
	void PresentBuffer(int16_t *out, Blip_Buffer *in);
	uint8_t GetData();
	void Step();

	uint8_t *mVgmData;
	int16_t *mTempBuffer;
	SnChip *mSN76489;
	VgmFileHeader *mFileHeader;
	uint32_t mWait;
	uint32_t mCycleCount, mSampleCycles;
	uint32_t mDataPos, mDataLen, mLoopPos;
	char mAsciiTitle[256], mAsciiAuthor[256], mAsciiComment[256];
};

MusicPlayer *VgmPlayerFactory();

#endif /* VGMPLAYER_H_ */
