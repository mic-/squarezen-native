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

	virtual int Prepare(std::wstring fileName);
	virtual int Run(uint32_t numSample, int16_t *buffer);
	virtual int Reset();

	YmChip mChip;
private:
	void PresentBuffer(int16_t *out, Blip_Buffer *in);

	uint8_t *mYmData;
	uint32_t mCycleCount, mFrameCycles;
	uint32_t mFrame, mNumFrames;
	uint32_t mDataOffs;
	uint8_t *mYmRegStream;
	uint8_t mYmRegs[16];

	int16_t *mTempBuffer;
};


#endif /* YMPLAYER_H_ */
