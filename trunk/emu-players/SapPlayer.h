/*
 * SapPlayer.h
 *
 *  Created on: Nov 12, 2013
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

#ifndef SAPPLAYER_H_
#define SAPPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "Emu6502.h"
#include "SapMapper.h"
#include "Pokey.h"

class SapPlayer : public MusicPlayer
{
public:
	SapPlayer();
	virtual ~SapPlayer();

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	virtual size_t GetNumChannels() const;
	virtual void GetChannelOutputs(int16_t *outputs) const;

	virtual void SetSubSong(uint32_t subSong);

private:
	Emu6502 *m6502;
	SapMapper *mMemory;
	Pokey *mPokey;
	uint8_t mFrameCycles, mCycleCount;
};

MusicPlayer *SapPlayerFactory();

#endif /* SAPPLAYER_H_ */

