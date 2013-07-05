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
#include <string.h>
#include "NativeLogger.h"
#include "NsfPlayer.h"


static float pulseTable[31];
static float tndTable[203];


NsfPlayer::NsfPlayer()
	: m6502(NULL)
	, m2A03(NULL)
	, mMemory(NULL)
{
	for (int i = 0; i < 31; i++) {
		pulseTable[i] = 95.52f / (8128.0f / (float)i + 100.0);
	}
	for (int i = 0; i < 203; i++) {
		tndTable[i] = 163.67f / (24329.0f / (float)i + 100.0);
	}
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
	NLOGV("NsfPlayer", "Reset");

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


void NsfPlayer::Execute6502(uint16_t address)
{
	//NLOGD("NsfPlayer", "Execute6502(%#x)", address);

	// JSR loadAddress
	mMemory->WriteByte(0x4f80, 0x20);
	mMemory->WriteByte(0x4f81, address & 0xff);
	mMemory->WriteByte(0x4f82, address >> 8);
	// -: JMP -
	mMemory->WriteByte(0x4f83, 0x4c);
	mMemory->WriteByte(0x4f84, 0x83);
	mMemory->WriteByte(0x4f85, 0x4f);
	m6502->mRegs.PC = 0x4f80;
	m6502->mCycles = 0;
	m6502->Run(mFrameCycles);
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
    	NLOGE("NsfPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    NLOGD("NsfPlayer", "Reading NSF header");
    musicFile.read((char*)&mFileHeader, sizeof(NsfFileHeader));
	if (!musicFile) {
		NLOGE("NsfPlayer", "Reading NSF header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    NLOGD("NsfPlayer", "ID: %c%c%c", mFileHeader.ID[0], mFileHeader.ID[1], mFileHeader.ID[2]);
    NLOGD("NsfPlayer", "Load: %#x\nInit: %#x\nPlay: %#x",
    		mFileHeader.loadAddress, mFileHeader.initAddress, mFileHeader.playAddress);
    if (strncmp(mFileHeader.ID, "NESM", 4)) {
    	NLOGE("NsfPlayer", "Bad NSF header signature");
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    mMetaData.SetTitle((char*)mFileHeader.title);
    mMetaData.SetAuthor((char*)mFileHeader.author);
    mMetaData.SetComment((char*)mFileHeader.copyright);

    //NLOGD("NsfPlayer", "Prepare(): title and author is %s, %s", mMetaData.GetTitle().c_str(), mMetaData.GetAuthor().c_str());

    bool usesBankswitching = false;
    for (i = 0; i < 8; i++) {
    	if (mFileHeader.initialBanks[i]) {
    		usesBankswitching = true;
    		break;
    	}
    }

	m6502 = new Emu6502;
	m2A03 = new Emu2A03;

	uint32_t offset = mFileHeader.loadAddress & 0x0fff;
	if (!usesBankswitching) {
		offset = mFileHeader.loadAddress - 0x8000;
	}

    numBanks = ((fileSize + offset - sizeof(NsfFileHeader)) + 0xfff) >> 12;
    NLOGD("NsfPlayer", "Trying to allocate %d bytes (file size = %d)", (uint32_t)numBanks << 12, fileSize);
	mMemory = new NsfMapper(numBanks);
	mMemory->SetApu(m2A03);

    NLOGD("NsfPlayer", "Loading to offset %#x", offset);
	musicFile.read((char*)(mMemory->GetRomPointer()) + offset,
			       fileSize - sizeof(NsfFileHeader));
	if (!musicFile) {
		NLOGE("NsfPlayer", "Read failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	NLOGD("NsfPlayer", "File read done");

	musicFile.close();

	m6502->SetMapper(mMemory);
	m2A03->SetMapper(mMemory);
	mMemory->Reset();
	m6502->Reset();

	NLOGD("NsfPlayer", "Initializing APU registers");

	m2A03->Reset();

	// Initialize the APU registers
	/*for (i = 0x4000; i < 0x4010; i++) {
		m2A03->Write(i, 0);
	}
	m2A03->Write(0x4010, 0x10);
	m2A03->Write(0x4011, 0x0);
	m2A03->Write(0x4012, 0x0);
	m2A03->Write(0x4013, 0x0);
	m2A03->Write(0x4015, 0x0f);*/

	// Set up ROM mapping
	for (int i = 0; i < 8; i++) {
		if (usesBankswitching) {
			mMemory->WriteByte(0x5FF8 + i, mFileHeader.initialBanks[i]);
		} else {
			mMemory->WriteByte(0x5FF8 + i, i);
		}
	}

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,82>[2]; //5];

	if (mBlipBuf->set_sample_rate(44100)) {
    	NLOGE("NsfPlayer", "Failed to set blipbuffer sample rate");
		return MusicPlayer::ERROR_UNKNOWN;
	}
	if (mFileHeader.region & NsfPlayer::REGION_PAL) {
		mBlipBuf->clock_rate(1662607);
		m2A03->SetClock(1662607, 50);
		mFrameCycles = 1662607 / 200;
	} else {
		mBlipBuf->clock_rate(1789773);
		m2A03->SetClock(1789773, 60);
		mFrameCycles = 1789773 / 240;
	}

	mCycleCount = 0;
	mPlayCounter = 0;

    // Setup waves
	for (int i = 0; i < 2; i++) {
		mSynth[i].volume(0.88); //0.17);
		mSynth[i].output(mBlipBuf);
	}

	mMetaData.SetNumSubSongs(mFileHeader.numSongs);
	mMetaData.SetDefaultSong(mFileHeader.firstSong);

	m6502->mRegs.S = 0xFF;
	SetSubSong(mFileHeader.firstSong - 1);

	NLOGD("NsfPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;

	return MusicPlayer::OK;
}


void NsfPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD("NsfPlayer", "SetSubSong(%d)", subSong);

	// Initialize the APU registers
	for (int i = 0x4000; i < 0x4010; i++) {
		m2A03->Write(i, 0);
	}
	m2A03->Write(0x4010, 0x10);
	m2A03->Write(0x4011, 0x0);
	m2A03->Write(0x4012, 0x0);
	m2A03->Write(0x4013, 0x0);
	m2A03->Write(0x4015, 0x0f);

	m6502->mRegs.A = subSong;
	m6502->mRegs.X = 0;  // NTSC/PAL
	Execute6502(mFileHeader.initAddress);
}


void NsfPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}


int NsfPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;
    int16_t pulseOut, tndOut;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	//NLOGV("NsfPlayer", "Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			if (mPlayCounter == 3) {
				Execute6502(mFileHeader.playAddress);
			}
			mPlayCounter = (mPlayCounter + 1) & 3;
		}

		m2A03->Step();

		pulseOut = (-m2A03->mChannels[Emu2A03::CHN_PULSE1].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_PULSE1].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_PULSE1].mOutputMask;
		pulseOut += (-m2A03->mChannels[Emu2A03::CHN_PULSE2].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_PULSE2].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_PULSE2].mOutputMask;
		pulseOut = (uint16_t)(pulseTable[pulseOut] * 82.0f);

		tndOut = ((-m2A03->mChannels[Emu2A03::CHN_TRIANGLE].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_TRIANGLE].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_TRIANGLE].mOutputMask) * 3;
		tndOut += ((-m2A03->mChannels[Emu2A03::CHN_NOISE].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_NOISE].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_NOISE].mOutputMask) * 2;
		tndOut += ((m2A03->mChannels[Emu2A03::CHN_DMC].mDuty)
					& m2A03->mChannels[Emu2A03::CHN_DMC].mOutputMask);
		tndOut = (uint16_t)(tndTable[tndOut] * 82.0f);

		if (pulseOut != m2A03->mChannels[0].mOut) {
			mSynth[0].update(k, pulseOut);
			m2A03->mChannels[0].mOut = pulseOut;
		}
		if (tndOut != m2A03->mChannels[1].mOut) {
			mSynth[1].update(k, tndOut);
			m2A03->mChannels[1].mOut = tndOut;
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
