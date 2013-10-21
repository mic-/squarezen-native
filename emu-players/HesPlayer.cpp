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
#define NLOG_TAG "HesPlayer"

#include <cstring>
#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "HesPlayer.h"


HesPlayer::HesPlayer()
	: m6280(NULL)
	, mPsg(NULL)
	, mMemory(NULL)
{
}

MusicPlayer *HesPlayerFactory()
{
	return new HesPlayer;
}

HesPlayer::~HesPlayer()
{
	delete m6280;
	delete mPsg;
	delete mMemory;
	m6280   = NULL;
	mPsg    = NULL;
	mMemory = NULL;
}

MusicPlayer::Result HesPlayer::Reset()
{
	NLOGV(NLOG_TAG, "Reset");

	delete m6280;
	delete mPsg;
	delete mMemory;
	m6280   = NULL;
	mPsg    = NULL;
	mMemory = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


MusicPlayer::Result HesPlayer::Prepare(std::string fileName)
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
		NLOGE(NLOG_TAG, "Reading HES header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.ID, "HESM", 4) ||
    		strncmp(mFileHeader.subID, "DATA", 4)) {
    	NLOGE(NLOG_TAG, "Bad HES header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

	// ToDo: finish

	NLOGV(NLOG_TAG, "File reading done");
	musicFile.close();

	m6280 = new HuC6280;
	mPsg = new HuC6280Psg;
	mMemory = new HesMapper(0);	// ToDo: set number of ROM banks

	mMemory->SetPsg(mPsg);
	mMemory->SetCpu(m6280);
	mMemory->SetPlayer(this);
	m6280->SetMapper(mMemory);

	mMemory->Reset();
	m6280->Reset();
	mPsg->Reset();

	for (int i = 0; i < 8; i++) {
		m6280->SetMpr(i, mFileHeader.MPR[i]);
		mMemory->SetMpr(i, mFileHeader.MPR[i]);
	}

	{
		mBlipBuf = new Blip_Buffer();
		mSynth = new Blip_Synth<blip_low_quality,4096>[3];	// Downmixed from the HuC6280's 6 channels

		if (mBlipBuf->set_sample_rate(44100)) {
			NLOGE(NLOG_TAG, "Failed to set blipbuffer sample rate");
			return MusicPlayer::ERROR_UNKNOWN;
		}
		mBlipBuf->clock_rate(3580000);

		for (int i = 0; i < 3; i++) {
			mSynth[i].output(mBlipBuf);
		}
	}
	{
		mBlipBufRight = new Blip_Buffer();
		mSynthRight = new Blip_Synth<blip_low_quality,4096>[3];

		if (mBlipBufRight->set_sample_rate(44100)) {
			//NativeLog("Failed to set blipbuffer sample rate");
			return MusicPlayer::ERROR_UNKNOWN;
		}
		mBlipBufRight->clock_rate(3580000);

		for (int i = 0; i < 3; i++) {
			mSynthRight[i].output(mBlipBufRight);
		}
	}

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void HesPlayer::Irq(uint8_t irqSource)
{
	if (HuC6280Mapper::TIMER_IRQ == irqSource) {
		m6280->Irq(0xFFFA);
		m6280->mCycles = 0;
		m6280->Run(m6280->mTimer.mCycles);
	} else if (HuC6280Mapper::VDC_IRQ == irqSource) {
		m6280->Irq(0xFFF8);
		m6280->mCycles = 0;
		m6280->Run(mFrameCycles);
	}
}


MusicPlayer::Result HesPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		m6280->mTimer.Step();

		mPsg->Step();

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mMemory->Irq(HuC6280Mapper::VDC_IRQ);
			mCycleCount = 0;
		}
	}

	return MusicPlayer::OK;
}
