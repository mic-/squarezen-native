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

#define NLOG_LEVEL_ERROR 0

#include <iostream>
#include <fstream>
#include <string.h>
#include "NativeLogger.h"
#include "SidPlayer.h"

#define BYTESWAP(w) w = (((w) & 0xFF00) >> 8) | (((w) & 0x00FF) << 8)
#define WORDSWAP(d) d = (((d) & 0xFFFF0000) >> 16) | (((d) & 0x0000FFFF) << 16)

SidPlayer::SidPlayer()
	: m6502(NULL)
	, mSid(NULL)
	, mMemory(NULL){
	// TODO: fill out
}


SidPlayer::~SidPlayer()
{
	delete m6502;
	delete mSid;
	delete mMemory;
	m6502 = NULL;
	mSid = NULL;
	mMemory = NULL;
}


int SidPlayer::Reset()
{
	NLOGV("NsfPlayer", "Reset");

	delete mBlipBuf;
	mBlipBuf = NULL;
	delete [] mSynth;
	mSynth = NULL;

	delete m6502;
	delete mSid;
	delete mMemory;
	m6502 = NULL;
	mSid = NULL;
	mMemory = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


int SidPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
    size_t fileSize;
    uint16_t *p16;

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NLOGE("SidPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    NLOGD("SidPlayer", "Reading PSID v1 header");
    musicFile.read((char*)&mFileHeader, 0x76);
	if (!musicFile) {
		NLOGE("SidPlayer", "Reading PSID v1 header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	BYTESWAP(mFileHeader.version);
	BYTESWAP(mFileHeader.dataOffset);
	BYTESWAP(mFileHeader.loadAddress);
	BYTESWAP(mFileHeader.initAddress);
	BYTESWAP(mFileHeader.playAddress);
	BYTESWAP(mFileHeader.numSongs);
	BYTESWAP(mFileHeader.firstSong);

	p16 = (uint16_t*)&mFileHeader.speed;
	BYTESWAP(*p16);
	p16++;
	BYTESWAP(*p16);
	WORDSWAP(mFileHeader.speed);

	if (mFileHeader.version == 2) {
	    NLOGD("SidPlayer", "Reading PSID v2 header fields");
	    musicFile.read(((char*)&mFileHeader) + 0x76, sizeof(mFileHeader) - 0x76);
		if (!musicFile) {
			NLOGE("SidPlayer", "Reading PSID v2 header fields failed");
	        musicFile.close();
			return MusicPlayer::ERROR_FILE_IO;
		}
	}

	i = musicFile.tellg();
	NLOGD("SidPlayer", "Data offset: %d, current position: %d", mFileHeader.dataOffset, i);

	if (mFileHeader.loadAddress == 0) {
		// First two bytes of data contain the load address
		musicFile.read((char*)&mFileHeader.loadAddress, 2);
		NLOGD("SidPlayer", "LOAD: %#x (0)", mFileHeader.loadAddress);
		fileSize -= 2;
	} else {
		NLOGD("SidPlayer", "LOAD: %#x", mFileHeader.loadAddress);
	}
	NLOGD("SidPlayer", "INIT: %#x", mFileHeader.initAddress);
	NLOGD("SidPlayer", "PLAY: %#x", mFileHeader.playAddress);

    mMetaData.SetTitle((char*)mFileHeader.title);
    mMetaData.SetAuthor((char*)mFileHeader.author);
    mMetaData.SetComment((char*)mFileHeader.copyright);

	mMemory = new SidMapper();

	musicFile.read((char*)mMemory->GetRamPointer() + mFileHeader.loadAddress, fileSize - mFileHeader.dataOffset);
	if (!musicFile) {
		NLOGE("SidPlayer", "Read failed");
		delete mMemory;
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	NLOGD("SidPlayer", "File read done");
	musicFile.close();

    if (!mFileHeader.initAddress) mFileHeader.initAddress = mFileHeader.loadAddress;
    /*if (!mFileHeader.playAddress) {
    	NLOGE("SidPlayer", "PlayAddress==0 is not supported");
		delete mMemory;
    	return MusicPlayer::ERROR_UNKNOWN;
    }*/

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,4096>[3];
	if (mBlipBuf->set_sample_rate(44100)) {
		NLOGE("SidPlayer", "Failed to set blipbuffer sample rate");
		delete mMemory;
		return MusicPlayer::ERROR_UNKNOWN;
	}
	mBlipBuf->clock_rate(1000000);
	for (i = 0; i < 3; i++) {
		//mSynth[i].volume(0.30);
		mSynth[i].output(mBlipBuf);
	}
	SetMasterVolume(0);

	mFrameCycles = 1000000 / 50;
	mCycleCount = 0;

	m6502 = new Emu6502;
	mSid = new Mos6581;
	mMemory->SetSid(mSid);
	mMemory->SetSidPlayer(this);
	m6502->SetMapper(mMemory);

	mMemory->Reset();
	m6502->Reset();
	mSid->Reset();

	mMetaData.SetNumSubSongs(mFileHeader.numSongs);
	mMetaData.SetDefaultSong(mFileHeader.firstSong);

	m6502->mRegs.S = 0xFF;
	SetSubSong(mFileHeader.firstSong - 1);

	NLOGD("SidPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SidPlayer::SetMasterVolume(int masterVol)
{
	NLOGV("SidPlayer", "SetMasterVolume(%d)", masterVol);

	for (int i = 0; i < 3; i++) {
		mSynth[i].volume(0.02 * (float)masterVol);
	}
}


void SidPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD("SidPlayer", "SetSubSong(%d)", subSong);

	/*mFrameCycles = 1000000 / ((mFileHeader.speed & (1 << subSong)) ? 60 : 50);
	NLOGE("SidPlayer", "Frame cycles = %d", mFrameCycles);*/

	m6502->mRegs.A = subSong;
	Execute6502(mFileHeader.initAddress);
}


void SidPlayer::Execute6502(uint16_t address)
{
	NLOGV("SidPlayer", "Execute6502(%#x)", address);

	if (address) {
		// JSR loadAddress
		mMemory->WriteByte(0x9ff0, 0x20);
		mMemory->WriteByte(0x9ff1, address & 0xff);
		mMemory->WriteByte(0x9ff2, address >> 8);
		// -: JMP -
		mMemory->WriteByte(0x9ff3, 0x4c);
		mMemory->WriteByte(0x9ff4, 0xf3);
		mMemory->WriteByte(0x9ff5, 0x9f);
		m6502->mRegs.PC = 0x9ff0;
		m6502->mCycles = 0;
		m6502->Run(mFrameCycles);
	} else {
		uint8_t bankSelect = mMemory->ReadByte(0x01) & 3;
		if (bankSelect >= 2) {
			m6502->SetBrkVector(0x314);
		}
		// BRK
		mMemory->WriteByte(0x9ff0, 0x00);
		// -: JMP -
		mMemory->WriteByte(0x9ff1, 0x4c);
		mMemory->WriteByte(0x9ff2, 0xf1);
		mMemory->WriteByte(0x9ff3, 0x9f);
		m6502->mRegs.PC = 0x9ff0;
		m6502->mCycles = 0;
		m6502->Run(mFrameCycles);
		m6502->SetBrkVector(0xfffe);
	}
}


void SidPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}


int SidPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			Execute6502(mFileHeader.playAddress);
		}

		mSid->Step();

		for (i = 0; i < 3; i++) {
			out = mSid->mChannels[i].mVol & mSid->mChannels[i].mOutputMask;
			//if (i != 2) out = 0;

			if (out != mSid->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mSid->mChannels[i].mOut = out;
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


