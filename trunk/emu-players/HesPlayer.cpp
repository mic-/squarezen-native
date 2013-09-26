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
#include "NativeLogger.h"
#include "HesPlayer.h"


HesPlayer::HesPlayer()
	: m6280(NULL)
	, mPsg(NULL)
	, mMemory(NULL)
{
}

HesPlayer::~HesPlayer()
{
	delete m6280;
	delete mPsg;
	delete mMemory;
	m6280   = NULL;
	mPsg    = NULL;
	mMemory = NULL;
}

int HesPlayer::Reset()
{
	// ToDo: implement
	NLOGV("HesPlayer", "Reset");

	delete m6280;
	delete mPsg;
	delete mMemory;
	m6280   = NULL;
	mPsg    = NULL;
	mMemory = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


int HesPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("HesPlayer", "Prepare(%s)", fileName.c_str());

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NLOGE("HesPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    NLOGV("HesPlayer", "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE("HesPlayer", "Reading HES header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.ID, "HESM", 4) ||
    		strncmp(mFileHeader.subID, "DATA", 4)) {
    	NLOGE("HesPlayer", "Bad HES header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

	// ToDo: finish

	NLOGV("HesPlayer", "File read done");
	musicFile.close();

	m6280 = new HuC6280;
	mPsg = new HuC6280Psg;
	mMemory = new HesMapper(0);	// ToDo: set number of ROM banks

	mMemory->SetPsg(mPsg);
	m6280->SetMapper(mMemory);

	mMemory->Reset();
	m6280->Reset();
	mPsg->Reset();

	for (int i = 0; i < 8; i++) {
		m6280->SetMpr(i, mFileHeader.MPR[i]);
		mMemory->SetMpr(i, mFileHeader.MPR[i]);
	}

	NLOGD("HesPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


int HesPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {

	}

	return MusicPlayer::OK;
}
