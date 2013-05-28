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

	virtual int Prepare(std::wstring fileName);
	virtual int Run(uint32_t numSample, int16_t *buffer);
	virtual int Reset();

private:
	void PresentBuffer(int16_t *out, Blip_Buffer *in);
	uint8_t GetData();
	void Step();

	uint32_t mWait;
	uint32_t mCycleCount, mSampleCycles;
	uint32_t mDataPos, mDataLen;
	uint8_t *mVgmData;
	int16_t *mTempBuffer;
	SnChip *mSN76489;
};

#endif /* VGMPLAYER_H_ */
