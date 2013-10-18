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

#include <cstring>
#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "KssPlayer.h"


KssPlayer::KssPlayer()
	: mZ80(NULL)
	, mMemory(NULL)
	, mAy(NULL)
	, mYM2413(NULL)
	, mSN76489(NULL)
	, mScc(NULL)
	, mSccEnabled(false)
{
}

MusicPlayer *KssPlayerFactory()
{
	return new KssPlayer;
}

KssPlayer::~KssPlayer()
{
	delete mZ80;
	delete mMemory;
	delete mAy;
	delete mYM2413;
	delete mSN76489;
	delete mScc;

	mZ80     = NULL;
	mMemory  = NULL;
	mAy      = NULL;
	mYM2413  = NULL;
	mSN76489 = NULL;
	mScc     = NULL;
}

void KssPlayer::SetSccEnabled(bool enabled)
{
	mSccEnabled = enabled;
}


MusicPlayer::Result KssPlayer::Reset()
{
	NLOGV("KssPlayer", "Reset");

	delete mBlipBuf;
	delete [] mSynth;
	mBlipBuf = NULL;
	mSynth   = NULL;

	delete mZ80;
	delete mMemory;
	delete mAy;
	delete mYM2413;
	delete mSN76489;
	delete mScc;

	mZ80     = NULL;
	mMemory  = NULL;
	mAy      = NULL;
	mYM2413  = NULL;
	mSN76489 = NULL;
	mScc     = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


MusicPlayer::Result KssPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("KssPlayer", "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    mIsKssx = false;

    NLOGV("KssPlayer", "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE("KssPlayer", "Reading KSS header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	if (strncmp(mFileHeader.ID, "KSSX", 4) == 0) {
		if (mFileHeader.reservedOrExtraHeader == 0x10) {
		    musicFile.read((char*)&mKssxHeader, sizeof(mKssxHeader));
			if (!musicFile) {
				NLOGE("KssPlayer", "Reading KSSX header failed");
		        musicFile.close();
				return MusicPlayer::ERROR_FILE_IO;
			}
			mIsKssx = true;
		}
	} else if (strncmp(mFileHeader.ID, "KSCC", 4)) {
		//ToDo: handle gzip-compressed KSSX files
    	NLOGE("KssPlayer", "Bad KSS header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    mZ80 = new Z80;
    mMemory = new KssMapper(mFileHeader.initDataSize + mFileHeader.loadAddress);
    mAy = new YmChip;
    mScc = new KonamiScc;
    int numSynths = 3+2;	// 3 for the AY, 2 for the SCC (pre-mixed from the SCC's 6 channels)
    if ((mFileHeader.extraChips & SN76489_MASK) == USES_SN76489) {
    	mSN76489 = new SnChip;
    	numSynths += 4;
    }
    if ((mFileHeader.extraChips & FMPAC_MASK) == USES_FMPAC) {
    	mYM2413 = new YM2413;
    	//numSynths += 3;		// pre-mixed from the YM2413's 9 channels
    }

    mFrameCycles = 3580000 / ((mIsKssx && ((mFileHeader.extraChips & KSSX_VSYNC_MASK) != USES_PAL_VSYNC)) ? 60 : 50);
    mCycleCount = 0;

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,4096>[numSynths];

    musicFile.read((char*)(mMemory->GetKssDataPointer()) + mFileHeader.loadAddress, mFileHeader.initDataSize);
	if (!musicFile) {
		NLOGE("KssPlayer", "Reading KSS data failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}
	NLOGV("KssPlayer", "File read done");
	musicFile.close();

	// ToDo: finish

	mZ80->mRegs.PC = mFileHeader.initAddress;
	mZ80->Run(mFrameCycles * 2);

	NLOGD("KssPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


MusicPlayer::Result KssPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;
	int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			mZ80->mRegs.PC = mFileHeader.playAddress;
			mZ80->Run(mFrameCycles);
		}

		if (mSccEnabled) {
			// ToDo: add SCC audio to blip synths
			mScc->Step();
		}

		if (mYM2413) {
			// ToDo: add YM2413 audio to blip synths
		}

		if (mSN76489) {
			// ToDo: add SN76489 audio to blip synths
			mSN76489->Step();

			for (int i = 0; i < 4; i++) {
				out = (-mSN76489->mChannels[i].mPhase) & SnChip::SN76489_VOL_TB[mSN76489->mChannels[i].mVol & 0x0F];

				if (out != mSN76489->mChannels[i].mOut) {
					mSynth[5 + i].update(k, out);
					mSN76489->mChannels[i].mOut = out;
				}
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


void KssPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}


