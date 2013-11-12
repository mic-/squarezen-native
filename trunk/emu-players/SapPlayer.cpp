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
#define NLOG_TAG "SapPlayer"

#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "SapPlayer.h"


SapPlayer::SapPlayer()
{

}


MusicPlayer *SapPlayerFactory()
{
	return new SapPlayer;
}


SapPlayer::~SapPlayer()
{
	delete m6502;
	delete mPokey;
	delete mMemory;

	m6502      = NULL;
	mPokey = NULL;
	mMemory    = NULL;
}


MusicPlayer::Result SapPlayer::Reset()
{
	// ToDo: implement
	NLOGV(NLOG_TAG, "Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

MusicPlayer::Result SapPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    // ToDo: finish

	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

	m6502 = new Emu6502;
	mPokey = new Pokey;

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SapPlayer::SetSubSong(uint32_t subSong)
{
	// ToDo: implement
}


MusicPlayer::Result SapPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}


size_t SapPlayer::GetNumChannels() const{
	// ToDo: implement
	return 0;
}


void SapPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}


