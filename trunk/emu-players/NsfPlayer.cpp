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

#include <iostream>
#include <fstream>
#include <string.h>
#include "NativeLogger.h"
#include "NsfPlayer.h"


NsfPlayer::NsfPlayer()
	: m6502(NULL)
	, m2A03(NULL)
	, mMemory(NULL)
{
	// TODO: fill out
}


NsfPlayer::~NsfPlayer()
{
	delete m6502;
	delete m2A03;
	delete mMemory;
	m6502 = NULL;
	m2A03 = NULL;
	mMemory = NULL;
}


int NsfPlayer::Reset()
{
	// TODO: fill out
	delete mBlipBuf;
	mBlipBuf = NULL;
	delete [] mSynth;
	mSynth = NULL;

	delete m6502;
	delete m2A03;
	delete mMemory;
	m6502 = NULL;
	m2A03 = NULL;
	mMemory = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

int NsfPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
    size_t fileSize;
    uint32_t numBanks;

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NativeLog(0, "NsfPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    NativeLog(0, "NsfPlayer", "Reading NSF header");
    musicFile.read((char*)&mFileHeader, sizeof(NsfFileHeader));
	if (!musicFile) {
		NativeLog(0, "NsfPlayer", "Reading NSF header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    NativeLog(0, "NsfPlayer", "ID: %c%c%c", mFileHeader.ID[0], mFileHeader.ID[1], mFileHeader.ID[2]);
    NativeLog(0, "NsfPlayer", "Load: %#x\nInit: %#x\nPlay: %#x",
    		mFileHeader.loadAddress, mFileHeader.initAddress, mFileHeader.playAddress);
    if (strncmp(mFileHeader.ID, "NESM", 4)) {
    	NativeLog(0, "NsfPlayer", "Bad NSF header signature");
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    bool usesBankswitching = false;
    for (i = 0; i < 8; i++) {
    	if (mFileHeader.initialBanks[i]) {
    		usesBankswitching = true;
    		break;
    	}
    }

	m6502 = new Emu6502;
	m2A03 = new Emu2A03;

    numBanks = ((fileSize + mFileHeader.loadAddress - sizeof(NsfFileHeader)) + 0xfff) >> 12;
    NativeLog(0, "NsfPlayer", "Trying to allocate %d bytes (filesize = %d)", (uint32_t)numBanks << 12, fileSize);
	mMemory = new NsfMapper(numBanks);

    NativeLog(0, "NsfPlayer", "Loading to offset %#x", (mFileHeader.loadAddress & ((usesBankswitching) ? 0x0fff : 0xffff)));
	musicFile.read((char*)(mMemory->GetRomPointer()) + (mFileHeader.loadAddress & ((usesBankswitching) ? 0x0fff : 0xffff)),
			       fileSize - sizeof(NsfFileHeader));
	if (!musicFile) {
		NativeLog(0, "NsfPlayer", "Read failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	NativeLog(0, "NsfPlayer", "File read done");

	musicFile.close();

	m6502->SetMapper(mMemory);
	mMemory->Reset();
	m6502->Reset();
	m2A03->Reset();

	// Set up ROM mapping
	for (int i = 0; i < 8; i++) {
		if (usesBankswitching) {
			mMemory->WriteByte(0x5FF8 + i, mFileHeader.initialBanks[i]);
		} else {
			mMemory->WriteByte(0x5FF8 + i, i);
		}
	}

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,82>[4];

	if (mBlipBuf->set_sample_rate(44100)) {
    	NativeLog(0, "NsfPlayer", "Failed to set blipbuffer sample rate");
		return MusicPlayer::ERROR_UNKNOWN;
	}
	if (mFileHeader.region & NsfPlayer::REGION_PAL) {
		mBlipBuf->clock_rate(1662607);
		mFrameCycles = 1662607 / 200;
	} else {
		mBlipBuf->clock_rate(1789773);
		mFrameCycles = 1789773 / 240;
	}

	mCycleCount = 0;
	mPlayCounter = 0;

    // Setup waves
	for (int i = 0; i < 4; i++) {
		mSynth[i].volume(0.22);
		mSynth[i].output(mBlipBuf);
	}

	NativeLog(0, "NsfPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;

	return MusicPlayer::OK;
}


int NsfPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	//NativeLog(0, "NsfPlayer", "Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			m2A03->Step();
			if (mPlayCounter == 3) {
				// TODO: call NSF PLAY routine
			}
			mPlayCounter = (mPlayCounter + 1) & 3;
		}

		m2A03->Step();

		for (i = 0; i < 4; i++) {
			out = (-m2A03->mChannels[i].mPhase) & Emu2A03::VOL_TB[m2A03->mChannels[i].mVol & 0x0F];

			if (out != m2A03->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				m2A03->mChannels[i].mOut = out;
			}
		}

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	//PresentBuffer(buffer, mBlipBuf);

	return MusicPlayer::OK;
}

