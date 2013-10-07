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
#define NLOG_TAG "SndhPlayer"

#include <cstring>
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


uint8_t *SndhPlayer::ParseTags(char *fileImage, size_t remainingBytes)
{
	bool headerEnd = false;
	char *endPointer = fileImage + remainingBytes;

	while ((fileImage < endPointer) && !headerEnd) {
		if (strncmp(fileImage, "HDNS", 4) == 0) {
			fileImage += 4;			// Skip past the tag
			headerEnd = true;
		} else if (strncmp(fileImage, "TITL", 4) == 0) {
			fileImage += 4;
			mMetaData.SetTitle(fileImage);
			while (*fileImage++);
		} else if (strncmp(fileImage, "COMM", 4) == 0) {
			fileImage += 4;
			mMetaData.SetAuthor(fileImage);
			while (*fileImage++);
		} else if (strncmp(fileImage, "RIPP", 4) == 0) {
			fileImage += 4;
			mMetaData.SetComment(fileImage);
			while (*fileImage++);
		} else if (strncmp(fileImage, "CONV", 4) == 0) {
			fileImage += 4;
			while (*fileImage++);	// Skip the string

		// ToDo: handle all other supported tags

		} else {
			NLOGE(NLOG_TAG, "Unsupported tag found: %c%c%c%c", fileImage[0], fileImage[1], fileImage[2], fileImage[3]);
			return NULL;
		}
	}

	return (uint8_t*)fileImage;
}


MusicPlayer::Result SndhPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    mMemory = new SndhMapper(fileSize);
    if (!mMemory) {
    	NLOGE(NLOG_TAG, "Failed to allocate memory for SNDH file");
    	musicFile.close();
    	return MusicPlayer::ERROR_OUT_OF_MEMORY;
    }
    uint8_t *fileImage = mMemory->GetFileImagePointer();

    NLOGV(NLOG_TAG, "Reading file");
    musicFile.read((char*)fileImage, fileSize);
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading SNDH file failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	musicFile.close();

	SndhFileHeader *header = (SndhFileHeader*)fileImage;
    if (strncmp(header->signature, "SNDH", 4)) {
    	NLOGE(NLOG_TAG, "Bad SNDH header signature");
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    if (NULL != ParseTags((char*)fileImage + sizeof(SndhFileHeader), fileSize - sizeof(SndhFileHeader))) {
        musicFile.close();
		return result;
    }

    m68k = new M68000;
    mYm = new YmChip;
    if (!m68k || !mYm) {
    	NLOGE(NLOG_TAG, "Failed to create emulator instance");
    	return MusicPlayer::ERROR_OUT_OF_MEMORY;
    }

	mYm->mEG.mEnvTable  = (uint16_t*)YmChip::YM2149_ENVE_TB;
	mYm->mEG.mMaxCycle = 31;

	m68k->Reset();
	mYm->Reset();
	mMemory->Reset();

    // ToDo: finish

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SndhPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD(NLOG_TAG, "SetSubSong(%d)", subSong);
	// ToDo: implement
}


MusicPlayer::Result SndhPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		// ToDo: run the m68k at appropriate intervals

		mYm->Step();

		for (i = 0; i < 3; i++) {
			out = (mYm->mChannels[i].mPhase | mYm->mChannels[i].mToneOff) &
				  (mYm->mNoise.mOut         | mYm->mChannels[i].mNoiseOff);
			out = (-out) & *(mYm->mChannels[i].mCurVol);

			if (out != mYm->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mYm->mChannels[i].mOut = out;
			}
		}
	}

	mBlipBuf->end_frame(blipLen);
	//PresentBuffer(buffer, mBlipBuf);

	return MusicPlayer::OK;
}

