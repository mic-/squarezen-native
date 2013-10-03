/*
 * SpcPlayer.h
 *
 *  Created on: Aug 12, 2013
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

#ifndef SPCPLAYER_H_
#define SPCPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "SSmp.h"
#include "SDsp.h"
#include "SpcMapper.h"

class SpcPlayer : public MusicPlayer
{
public:
	SpcPlayer();
	virtual ~SpcPlayer();

	virtual int Prepare(std::string fileName);
	virtual int Run(uint32_t numSamples, int16_t *buffer);
	virtual int Reset();

	typedef struct __attribute__ ((__packed__))
	{
		char signature[33];
		uint8_t reserved1[2];	// 26, 26
		uint8_t id666Present;	// 26 = contains ID666 tag
		uint8_t minorVersion;

		uint16_t regPC;
		uint8_t regA;
		uint8_t regX;
		uint8_t regY;
		uint8_t regPSW;
		uint8_t regSP;
		uint8_t reserved2[2];

		// ID666 tag
		char title[32];
		char game[32];
		char dumper[16];
		char comment[32];
		char dumpDate[11];
		char lengthSeconds[3];
		char fadeMs[5];
		char artist[32];
		uint8_t chanDisable;
		uint8_t emulatorUsed;
		char reserved[45];
	} SpcFileHeader;

private:
	void ExecuteSSmp();

	SSmp *mSSmp;
	SDsp *mSDsp;
	SpcMapper *mMemory;
	Blip_Buffer *mBlipBufRight;
	Blip_Synth<blip_low_quality,4096> *mSynthRight;
	uint8_t mDspRegisterInit[128];
	SpcFileHeader mFileHeader;
	uint32_t mCycleCount, mScanlineCycles;
};

MusicPlayer *SpcPlayerFactory();

#endif /* SPCPLAYER_H_ */
