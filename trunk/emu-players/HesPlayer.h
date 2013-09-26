/*
 * HesPlayer.h
 *
 *  Created on: Aug 30, 2013
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

#ifndef HESPLAYER_H_
#define HESPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "HuC6280.h"
#include "HesMapper.h"


class HesPlayer : public MusicPlayer
{
public:
	HesPlayer();
	virtual ~HesPlayer();

	virtual int Prepare(std::string fileName);
	virtual int Run(uint32_t numSamples, int16_t *buffer);
	virtual int Reset();

	typedef struct __attribute__ ((__packed__))
	{
		char ID[4];
		uint8_t version;
		uint8_t firstSong;
		uint16_t initAddress;
		uint8_t MPR[8];
		char subID[4];
		uint32_t dataSize;
		uint32_t loadAddress;
		uint32_t reserved;
	} HesFileHeader;

private:
	HesFileHeader mFileHeader;
	HuC6280 *m6280;
	HuC6280Psg *mPsg;
	HesMapper *mMemory;
};

#endif	/* HESPLAYER_H_ */
