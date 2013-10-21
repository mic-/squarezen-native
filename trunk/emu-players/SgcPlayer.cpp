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
	, mYM2413(NULL)
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
	delete mYM2413;
	delete mMemory;
	mZ80 = NULL;
	mSN76489 = NULL;
	mYM2413 = NULL;
	mMemory = NULL;
}

MusicPlayer::Result SgcPlayer::Reset()
{
	NLOGV(NLOG_TAG, "Reset()");

	delete mZ80;
	delete mSN76489;
	delete mYM2413;
	delete mMemory;
	mZ80 = NULL;
	mSN76489 = NULL;
	mYM2413 = NULL;
	mMemory = NULL;

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
	if (mFileHeader.systemType == SgcPlayer::SYSTEM_SMS) {
		mYM2413 = new YM2413;
	}
	mZ80->SetMapper(mMemory);
	mMemory->SetPsg(mSN76489);
	mMemory->SetYM2413(mYM2413);
	mMemory->SetSystemType(mFileHeader.systemType);

	mMemory->Reset();
	mZ80->Reset();
	mSN76489->Reset();

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,4096>[4];
	if (mBlipBuf->set_sample_rate(44100)) {
    	NLOGE(NLOG_TAG, "Failed to set blipbuffer sample rate");
		return MusicPlayer::ERROR_UNKNOWN;
	}
	mBlipBuf->clock_rate(3579545);

	mFrameCycles = 3579545 / ((mFileHeader.region == SgcPlayer::REGION_PAL) ? 50 : 60);
	mCycleCount = 0;

    // Setup waves
	for (int i = 0; i < 4; i++) {
		mSynth[i].volume(0.22);
		mSynth[i].output(mBlipBuf);
	}

	if (mFileHeader.systemType <= SgcPlayer::SYSTEM_GG) {
		for (int i = 0; i < 4; i++) {
			mMemory->WriteByte(0xFFFC + i, mFileHeader.mapperInit[i]);
		}
	}

	mZ80->mRegs.SP = mFileHeader.stackPointer;
	SetSubSong(mFileHeader.firstSong - 1);

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SgcPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD(NLOG_TAG, "SetSubSong(%d)", subSong);

	mZ80->mRegs.PC = mFileHeader.initAddress;
	mZ80->mRegs.A = subSong;
	mZ80->Run(mFrameCycles * 2);
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
		if (0 == mCycleCount) {
			mZ80->mRegs.PC = mFileHeader.playAddress;
			mZ80->Run(mFrameCycles);
		}

		mSN76489->Step();

		for (i = 0; i < 4; i++) {
			out = (-mSN76489->mChannels[i].mPhase) & SnChip::SN76489_VOL_TB[mSN76489->mChannels[i].mVol & 0x0F];

			if (out != mSN76489->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mSN76489->mChannels[i].mOut = out;
			}
		}

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf);

	return MusicPlayer::OK;
}

