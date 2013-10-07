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
#define NLOG_TAG "SgcPlayer"

#include <cstring>
#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "SgcPlayer.h"


SgcPlayer::SgcPlayer()
	: mZ80(NULL)
	, mSN76489(NULL)
	, mMemory(NULL)
{
}

MusicPlayer *SgcPlayerFactory()
{
	return new SgcPlayer;
}

SgcPlayer::~SgcPlayer()
{
	delete mZ80;
	delete mSN76489;
	delete mMemory;
	mZ80 = NULL;
	mSN76489 = NULL;
	mMemory = NULL;
}

MusicPlayer::Result SgcPlayer::Reset()
{
	// ToDo: implement
	NLOGV(NLOG_TAG, "Reset()");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


MusicPlayer::Result SgcPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    NLOGV(NLOG_TAG, "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading SGC header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.signature, "SGC", 3)) {
    	NLOGE(NLOG_TAG, "Bad SGC header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

	mMetaData.SetTitle(mFileHeader.title);
	mMetaData.SetAuthor(mFileHeader.artist);
	mMetaData.SetComment(mFileHeader.copyright);

	// ToDo: finish

	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

	mZ80 = new Z80;
	mSN76489 = new SnChip;
	mMemory = new SgcMapper(0);	// ToDo: set number of ROM banks

	mZ80->SetMapper(mMemory);
	mMemory->SetPsg(mSN76489);
	mMemory->SetSystemType(mFileHeader.systemType);

	mMemory->Reset();
	mZ80->Reset();
	mSN76489->Reset();

	if (mFileHeader.systemType <= SgcPlayer::SYSTEM_GG) {
		for (int i = 0; i < 4; i++) {
			mMemory->WriteByte(0xFFFC + i, mFileHeader.mapperInit[i]);
		}
	}

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SgcPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD(NLOG_TAG, "SetSubSong(%d)", subSong);
	// ToDo: implement
}


void SgcPlayer::ExecuteZ80(uint16_t address)
{
	// ToDo: implement
}


void SgcPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}


MusicPlayer::Result SgcPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		// ToDo: run the Z80 at appropriate intervals

		mSN76489->Step();

		for (i = 0; i < 4; i++) {
			out = (-mSN76489->mChannels[i].mPhase) & SnChip::SN76489_VOL_TB[mSN76489->mChannels[i].mVol & 0x0F];

			if (out != mSN76489->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mSN76489->mChannels[i].mOut = out;
			}
		}
	}

	mBlipBuf->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf);

	return MusicPlayer::OK;
}

