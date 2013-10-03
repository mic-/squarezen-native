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
#include "AyPlayer.h"


AyPlayer::AyPlayer()
{

}

AyPlayer::~AyPlayer()
{

}

int AyPlayer::Reset()
{
	// ToDo: implement
	NLOGV("AyPlayer", "Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

int AyPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("AyPlayer", "Prepare(%s)", fileName.c_str());

    int result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    // ToDo: finish

	NLOGV("AyPlayer", "File read done");
	musicFile.close();

	NLOGD("AyPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


int AyPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}
