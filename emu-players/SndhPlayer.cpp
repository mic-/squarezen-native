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
#include "SndhPlayer.h"


SndhPlayer::SndhPlayer()
	: m68k(NULL)
	, mYm(NULL)
	, mMemory(NULL)
{
}

MusicPlayer *SndhPlayerFactory()
{
	return new SndhPlayer;
}

SndhPlayer::~SndhPlayer()
{
	delete m68k;
	delete mYm;
	delete mMemory;
	m68k = NULL;
	mYm = NULL;
	mMemory = NULL;
}

MusicPlayer::Result SndhPlayer::Reset()
{
	// ToDo: implement
	NLOGV("SndhPlayer", "Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

MusicPlayer::Result SndhPlayer::ParseTrackHeader()
{
	return MusicPlayer::OK;
}


MusicPlayer::Result SndhPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("SndhPlayer", "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    NLOGV("SndhPlayer", "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE("SndhPlayer", "Reading SNDH header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.signature, "SNDH", 4)) {
    	NLOGE("SndhPlayer", "Bad SNDH header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    // ToDo: finish

	NLOGD("SndhPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


MusicPlayer::Result SndhPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}

