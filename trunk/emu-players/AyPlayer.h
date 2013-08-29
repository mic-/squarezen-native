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
#include "YM2149.h"
#include "MusicPlayer.h"
#include "Z80.h"


class AyPlayer : public MusicPlayer {
public:
	AyPlayer();
	virtual ~AyPlayer();

	virtual int Prepare(std::string fileName);
	virtual int Run(uint32_t numSamples, int16_t *buffer);
	virtual int Reset();

private:
	Z80 *mZ80;
	YmChip mChip;
};


#endif /* AYPLAYER_H_ */
