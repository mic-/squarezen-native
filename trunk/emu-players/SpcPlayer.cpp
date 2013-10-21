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
#include "SpcPlayer.h"


SpcPlayer::SpcPlayer()
	: mBlipBufRight(NULL), mSynthRight(NULL)
{

}

MusicPlayer *SpcPlayerFactory()
{
	return new SpcPlayer;
}

SpcPlayer::~SpcPlayer()
{
	delete mBlipBufRight;
	delete [] mSynthRight;

	mBlipBufRight = NULL;
	mSynthRight   = NULL;
}


MusicPlayer::Result SpcPlayer::Reset()
{
	NLOGV("SpcPlayer", "SpcPlayer::Reset");

	delete mBlipBuf;
	delete [] mSynth;
	mBlipBuf = NULL;
	mSynth   = NULL;

	delete mBlipBufRight;
	delete [] mSynthRight;
	mBlipBufRight = NULL;
	mSynthRight   = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

MusicPlayer::Result SpcPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("SpcPlayer", "SpcPlayer::Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    NLOGV("SpcPlayer", "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE("SpcPlayer", "Reading SPC header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	mMemory = new SpcMapper;
	mSSmp = new SSmp;
	mSDsp = new SDsp;
	mMemory->SetCpu(mSSmp);
	mMemory->SetDsp(mSDsp);
	mSSmp->SetMapper(mMemory);

	if (mFileHeader.id666Present == 26) {
		mMetaData.SetTitle(mFileHeader.title);
		mMetaData.SetAuthor(mFileHeader.artist);
		mMetaData.SetComment(mFileHeader.game);
	}

    musicFile.read((char*)mMemory->mRam, 0x10000);
	if (!musicFile) {
		NLOGE("SpcPlayer", "Reading SPC RAM dump failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    musicFile.read((char*)mDspRegisterInit, 128);
	if (!musicFile) {
		NLOGE("SpcPlayer", "Reading DSP register values failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	NLOGV("SpcPlayer", "File read done");
	musicFile.close();

	// Left output
	{
		mBlipBuf = new Blip_Buffer();
		mSynth = new Blip_Synth<blip_low_quality,4096>[4];

		if (mBlipBuf->set_sample_rate(44100)) {
			NLOGE("SpcPlayer", "Failed to set blipbuffer sample rate");
			return MusicPlayer::ERROR_UNKNOWN;
		}
		mBlipBuf->clock_rate(SDsp::S_DSP_CLOCK / SDsp::S_DSP_CLOCK_DIVIDER);

		for (int i = 0; i < 4; i++) {
			//mSynth[i].volume(0.22);
			mSynth[i].output(mBlipBuf);
		}
	}
	// Right output
	{
		mBlipBufRight = new Blip_Buffer();
		mSynthRight = new Blip_Synth<blip_low_quality,4096>[4];

		if (mBlipBufRight->set_sample_rate(44100)) {
			return MusicPlayer::ERROR_UNKNOWN;
		}
		mBlipBufRight->clock_rate(SDsp::S_DSP_CLOCK / SDsp::S_DSP_CLOCK_DIVIDER);

		for (int i = 0; i < 4; i++) {
			mSynthRight[i].output(mBlipBufRight);
		}
	}

	mMemory->Reset();
	mSSmp->Reset();
	//mSDsp->Reset();

	for (int i = 0; i < 128; i++) {
		mSDsp->Write(i, mDspRegisterInit[i]);
	}

	mScanlineCycles = SSmp::S_SMP_CLOCK / (60 * 262);
	mCycleCount = 0;

	mSSmp->mRegs.PC = mFileHeader.regPC;
	mSSmp->mRegs.A = mFileHeader.regA;
	mSSmp->mRegs.X = mFileHeader.regX;
	mSSmp->mRegs.Y = mFileHeader.regY;
	mSSmp->mRegs.SP = mFileHeader.regSP;
	mSSmp->mRegs.PSW = mFileHeader.regPSW;
	/*mSSmp->mCycles = 0;
	mSSmp->Run(500);*/

	NLOGD("SpcPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SpcPlayer::ExecuteSSmp()
{
	mSSmp->Run(mScanlineCycles);
}


MusicPlayer::Result SpcPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	int k;
	int blipLen = mBlipBuf->count_clocks(numSamples);
	int dspDivider = 0;
	int16_t out, outL, outR;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			// ToDo: it would perhaps be nice with a lower granularity for the SMP/DSP emulation cycle ratio
			ExecuteSSmp();
		}

		dspDivider++;
		if (dspDivider == SDsp::S_DSP_CLOCK_DIVIDER) {
			dspDivider = 0;
			mSDsp->Step();
		}

		mCycleCount++;
		if (mCycleCount == mScanlineCycles) {
			mCycleCount = 0;
		}
	}

	return MusicPlayer::OK;
}
