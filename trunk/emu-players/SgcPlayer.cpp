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
#include "SgcPlayer.h"


SgcPlayer::SgcPlayer()
	: mZ80(NULL)
	, mMemory(NULL)
{
}

SgcPlayer::~SgcPlayer()
{
	delete mZ80;
	delete mMemory;
	mZ80 = NULL;
	mMemory = NULL;
}

int SgcPlayer::Reset()
{
	// ToDo: implement
	NLOGV("SgcPlayer", "SgcPlayer::Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


int SgcPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("SgcPlayer", "Prepare(%s)", fileName.c_str());

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NLOGE("SgcPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    NLOGV("SgcPlayer", "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE("SgcPlayer", "Reading SGC header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.signature, "SGC", 3)) {
    	NLOGE("SgcPlayer", "Bad SGC header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

	mMetaData.SetTitle(mFileHeader.title);
	mMetaData.SetAuthor(mFileHeader.artist);
	mMetaData.SetComment(mFileHeader.copyright);

	// ToDo: finish

	NLOGV("SgcPlayer", "File read done");
	musicFile.close();

	NLOGD("SgcPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


int SgcPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}

