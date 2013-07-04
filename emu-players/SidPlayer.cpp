/*
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

#define NLOG_LEVEL_VERBOSE 0

#include <iostream>
#include <fstream>
#include <string.h>
#include "NativeLogger.h"
#include "SidPlayer.h"

SidPlayer::SidPlayer()
	: m6502(NULL)
	, mSid(NULL)
	, mMemory(NULL){
	// TODO: fill out
}


SidPlayer::~SidPlayer()
{
	delete m6502;
	delete mSid;
	delete mMemory;
	m6502 = NULL;
	mSid = NULL;
	mMemory = NULL;
}


int SidPlayer::Reset()
{
	NLOGV("NsfPlayer", "Reset");

	delete mBlipBuf;
	mBlipBuf = NULL;
	delete [] mSynth;
	mSynth = NULL;

	delete m6502;
	delete mSid;
	delete mMemory;
	m6502 = NULL;
	mSid = NULL;
	mMemory = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


int SidPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
    size_t fileSize;
    uint32_t numBanks;

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NLOGE("SidPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    NLOGD("SidPlayer", "Reading PSID v1 header");
    musicFile.read((char*)&mFileHeader, 0x76);
	if (!musicFile) {
		NLOGE("SidPlayer", "Reading PSID v1 header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	if (mFileHeader.version == 2) {
	    NLOGD("SidPlayer", "Reading PSID v2 header fields");
	    musicFile.read(((char*)&mFileHeader) + 0x76, sizeof(mFileHeader) - 0x76);
		if (!musicFile) {
			NLOGE("SidPlayer", "Reading PSID v2 header fields failed");
	        musicFile.close();
			return MusicPlayer::ERROR_FILE_IO;
		}
	}

	musicFile.close();

	// TODO: finish

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


int SidPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	// TODO: finish

	return MusicPlayer::OK;
}


