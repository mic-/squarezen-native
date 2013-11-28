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
#include <math.h>
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

	delete mBlipBufRight;
	delete [] mSynthRight;
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

	uint32_t romBanks = (mFileHeader.dataSize + mFileHeader.loadAddress + 0x1FFF) / 0x2000;
	mMemory = new HesMapper(romBanks);

	NLOGD(NLOG_TAG, "Loading %d bytes at %#x (romBanks = %d)", mFileHeader.dataSize, mFileHeader.loadAddress, romBanks);
	musicFile.read((char*)(mMemory->GetRomPointer()) + mFileHeader.loadAddress, mFileHeader.dataSize);
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Read failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	NLOGV(NLOG_TAG, "File reading done");
	musicFile.close();

	m6280 = new HuC6280;
	mPsg = new HuC6280Psg;

	mMemory->SetPsg(mPsg);
	mMemory->SetCpu(m6280);
	mMemory->SetPlayer(this);
	mPsg->SetPlayer(this);
	m6280->SetMapper(mMemory);

	mMemory->Reset();
	m6280->Reset();
	mPsg->Reset();

	// Set up memory paging registers
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
			mSynth[i].volume(0.05);
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
			mSynthRight[i].volume(0.05);
		}
	}

	mFrameCycles = 1790000 / 60; //3580000 / 60;
	mCycleCount = 0;

	//SetMasterVolume(0, 0);
	SetSubSong(0);


	/*if (mFileHeader.loadAddress >= 0x20) {
		NLOGD(NLOG_TAG, "Running CPU for %d cycles", mFrameCycles*2);
		uint8_t *driver = mMemory->GetRomPointer();
		driver[0] = 0x4c;
		driver[1] = mFileHeader.initAddress & 0xff;
		driver[2] = mFileHeader.initAddress >> 8;
		// -: JMP -
		driver[3] = 0x4c;
		driver[4] = 0x03;
		driver[5] = 0x00;
		m6280->mRegs.PC = 0;
		m6280->Run(mFrameCycles * 2);
	}*/

	//m6280->mRegs.PC = mMemory->ReadByte(0xFFFE);
	//m6280->mRegs.PC |= (uint16_t)mMemory->ReadByte(0xFFFF) << 8;

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void HesPlayer::SetSubSong(uint32_t subSong)
{
	m6280->mRegs.PC = mFileHeader.initAddress;
	m6280->mRegs.A = subSong;
	m6280->mCycles = 0;
	m6280->Run(mFrameCycles * 10);		// This is a hack
}


void HesPlayer::Irq(uint8_t irqSource)
{
	if (m6280->mRegs.F & HuC6280::FLAG_I) {
		return;
	}

	if (HuC6280Mapper::TIMER_IRQ == irqSource) {
		NLOGD(NLOG_TAG, "Timer IRQ");
		m6280->Irq(0xFFFA);
		//m6280->mCycles = 0;
		//m6280->Run(m6280->mTimer.mCycles);
	} else if (HuC6280Mapper::VDC_IRQ == irqSource) {
		m6280->Irq(0xFFF8);
		//m6280->mCycles = 0;
		//m6280->Run(mFrameCycles);
	}
}


void HesPlayer::SetMasterVolume(int left, int right)
{
	NLOGD(NLOG_TAG, "SetMasterVolume(%d, %d)", left, right);

	double l = pow(10.0, ((double)(left - 15)) / 6.67) * 0.4;
	double r = pow(10.0, ((double)(right - 15)) / 6.67) * 0.4;

	for (int i = 0; i < 3; i++) {
		mSynth[i].volume(l);
		mSynthRight[i].volume(r);
	}
}


MusicPlayer::Result HesPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;
	static int16_t outL[3] = {-1, -1, -1};
	static int16_t outR[3] = {-1, -1, -1};

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

    NLOGD(NLOG_TAG, "Run(%d)", numSamples);

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		if (k & 1) m6280->mTimer.Step();

		mPsg->Step();

		for (int i = 0; i < 6; i += 2) {
			int32_t left = mPsg->mChannels[i].mOut * HuC6280Psg::HUC6280_VOL_TB[mPsg->mChannels[i].mVolL];		// 5bit * 12bit = 17bit
			left += mPsg->mChannels[i + 1].mOut * HuC6280Psg::HUC6280_VOL_TB[mPsg->mChannels[i + 1].mVolL];
			left >>= 6;	// 18bit -> 12bit
			if (left != outL[i >> 1]) {
				mSynth[i >> 1].update(k, left);
				outL[i >> 1] = left;
			}

			int32_t right = mPsg->mChannels[i].mOut * HuC6280Psg::HUC6280_VOL_TB[mPsg->mChannels[i].mVolR];		// 5bit * 12bit = 17bit
			right += mPsg->mChannels[i + 1].mOut * HuC6280Psg::HUC6280_VOL_TB[mPsg->mChannels[i + 1].mVolR];
			right >>= 6;	// 18bit -> 12bit
			if (right != outR[i >> 1]) {
				mSynthRight[i >> 1].update(k, right);
				outR[i >> 1] = right;
			}
		}

		if (k & 1) mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mMemory->Irq(HuC6280Mapper::VDC_IRQ);
			m6280->mCycles = 0;
			m6280->Run(mCycleCount);
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	mBlipBufRight->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf, mBlipBufRight);

	return MusicPlayer::OK;
}


void HesPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}
