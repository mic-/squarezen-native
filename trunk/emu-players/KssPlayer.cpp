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
#define NLOG_TAG "KssPlayer"

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
	NLOGV(NLOG_TAG, "Reset");

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

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    mIsKssx = false;

    NLOGV(NLOG_TAG, "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading KSS header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	if (strncmp(mFileHeader.ID, "KSSX", 4) == 0) {
		if (mFileHeader.reservedOrExtraHeader == 0x10) {
		    musicFile.read((char*)&mKssxHeader, sizeof(mKssxHeader));
			if (!musicFile) {
				NLOGE(NLOG_TAG, "Reading KSSX header failed");
		        musicFile.close();
				return MusicPlayer::ERROR_FILE_IO;
			}
			mIsKssx = true;
		}
	} else if (strncmp(mFileHeader.ID, "KSCC", 4)) {
		//ToDo: handle gzip-compressed KSSX files
    	NLOGE(NLOG_TAG, "Bad KSS header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    mZ80 = new Z80;

    mMemory = new KssMapper(mFileHeader.initDataSize + mFileHeader.loadAddress);
    memset(mMemory->GetRamPointer(), 0, 64*1024);
    memset(mMemory->GetRamPointer(), 0xC9, 0x4000);	// Fill the first 16kB with opcode C9 (RET)

    mAy = new YmChip(YmChip::AY_3_8910_ENVELOPE_STEPS);
    mScc = new KonamiScc;
    int numSynths = 3+2;	// 3 for the AY, 2 for the SCC (pre-mixed from the SCC's 5 channels)
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

	// ToDo: read straight into RAM?
    musicFile.read((char*)(mMemory->GetKssDataPointer()) + mFileHeader.loadAddress, mFileHeader.initDataSize);
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading KSS data failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}
	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

	// ToDo: finish

	SetSubSong(mIsKssx ? (mKssxHeader.firstSong - 1) : 0);

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void KssPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD(NLOG_TAG, "Setting subsong %d", subSong);

	mZ80->mRegs.SP = 0xF380;
	mZ80->mRegs.A = subSong;
	mZ80->mRegs.PC = mFileHeader.initAddress;
	mZ80->Run(mFrameCycles * 2);
}


MusicPlayer::Result KssPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;
	int16_t out;
	static int16_t kssOut1 = -1, kssOut2 = -1;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			mZ80->mRegs.PC = mFileHeader.playAddress;
			mZ80->Run(mFrameCycles);
		}

		mAy->Step();
		for (int i = 0; i < 3; i++) {
			out = (mAy->mChannels[i].mPhase | mAy->mChannels[i].mToneOff) &
				  (mAy->mNoise.mOut         | mAy->mChannels[i].mNoiseOff);
			out = (-out) & *(mAy->mChannels[i].mCurVol);

			if (out != mAy->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mAy->mChannels[i].mOut = out;
			}
		}

		if (mSccEnabled) {
			mScc->Step();
			uint16_t out1 = (mScc->mChannels[0].mOut * mScc->mChannels[0].mVol)
					      + (mScc->mChannels[1].mOut * mScc->mChannels[1].mVol)
					      + (mScc->mChannels[2].mOut * mScc->mChannels[2].mVol);

			uint16_t out2 = (mScc->mChannels[3].mOut * mScc->mChannels[3].mVol)
					      + (mScc->mChannels[4].mOut * mScc->mChannels[4].mVol);

			out1 = (out1 * 85) >> 8;	// out1 /= 3
			out2 >>= 1;					// out2 /= 2

			if (out1 != kssOut1) {
				mSynth[3].update(k, out1);
				kssOut1 = out1;
			}
			if (out2 != kssOut2) {
				mSynth[4].update(k, out2);
				kssOut2 = out2;
			}
		}

		if (mYM2413) {
			// ToDo: add YM2413 audio to blip synths
			// mYM2413->Step();
		}

		if (mSN76489) {
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


size_t KssPlayer::GetNumChannels() const
{
	// ToDo: implement
	return 0;
}

void KssPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}


