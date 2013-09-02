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
#include "KssPlayer.h"


KssPlayer::KssPlayer()
	: mZ80(NULL)
	, mMemory(NULL)
	, mAy(NULL)
	, mSN76489(NULL)
	, mScc(NULL)
	, mSccEnabled(false)
{
}

KssPlayer::~KssPlayer()
{
	delete mZ80;
	delete mMemory;
	delete mAy;
	delete mSN76489;
	delete mScc;

	mZ80     = NULL;
	mMemory  = NULL;
	mAy      = NULL;
	mSN76489 = NULL;
	mScc     = NULL;
}

void KssPlayer::SetSccEnabled(bool enabled)
{
	mSccEnabled = enabled;
}


int KssPlayer::Reset()
{
	// ToDo: implement
	NLOGV("KssPlayer", "Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


int KssPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("KssPlayer", "Prepare(%s)", fileName.c_str());

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NLOGE("KssPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    NLOGV("KssPlayer", "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE("KssPlayer", "Reading KSS header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.ID, "KSCC", 4)) {
    	NLOGE("KssPlayer", "Bad KSS header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    mZ80 = new Z80;
    mMemory = new KssMapper(0);	// ToDo: set number of ROM banks
    mAy = new YmChip;
    if ((mFileHeader.extraChips & SN76489_MASK) == USES_SN76489) {
    	mSN76489 = new SnChip;
    }
    mScc = new KonamiScc;

	// ToDo: finish

	NLOGV("KssPlayer", "File read done");
	musicFile.close();

	NLOGD("KssPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


int KssPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {

	}

	mBlipBuf->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf);

	return MusicPlayer::OK;
}


void KssPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}


