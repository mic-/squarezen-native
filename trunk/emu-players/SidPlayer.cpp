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

#define NLOG_LEVEL_DEBUG 0

#include <iostream>
#include <fstream>
#include <string.h>
#include "NativeLogger.h"
#include "SidPlayer.h"

#define BYTESWAP(w) w = (((w) & 0xFF00) >> 8) | (((w) & 0x00FF) << 8)
#define WORDSWAP(d) d = (((d) & 0xFFFF0000) >> 16) | (((d) & 0x0000FFFF) << 16)


static const uint8_t SID_DRIVER[] = {
	0x78,  				// 00: SEI
	0xA9, 0x7F,  		// 01: LDA  #$7F
	0x8D, 0x0D, 0xDC,	// 03: STA $DC0D
	0x8D, 0x0D, 0xDD,	// 06: STA $DD0D
	0xA9, 0x08,			// 09: LDA #$08
	0x8D, 0x0E, 0xDC,	// 0B: STA $DC0E
	0x8D, 0x0E, 0xDD,	// 0E: STA $DD0E
	0x8D, 0x0F, 0xDC,	// 11: STA $DC0F
	0x8D, 0x0F, 0xDD,	// 14: STA $DD0F

	0xA9, 0x20,			// 17: LDA #$25
	0x8D, 0x04, 0xDC,	// 19: STA $DC04
	0xA9, 0x4E,			// 1C: LDA #$40
	0x8D, 0x05, 0xDC,	// 1E: STA $DC05

	0xA9, 0x81,			// 21: LDA #$81
	0x8D, 0x0D, 0xDC,	// 23: STA $DC0D
	0xA9, 0x19,			// 26: LDA #$19
	0x8D, 0x0E, 0xDC,	// 28: STA $DC0E

	0xA9, 0x36,			// 2B: LDA #$36
	0x85, 0x01,			// 2D: STA $01

	0xA9, 0x00,			// 2F: LDA #song		  (song is filled in later)
	0x20, 0x00, 0x00,	// 31: JSR init_address   (address is filled in later)
	0x58,				// 34: CLI
	0x4C, 0x35, 0x00,	// 35: JMP 35

	0x20, 0x00, 0x00,	// 38: JSR play_address   (address is filled in later)
	0xAD, 0x0D, 0xDC,	// 3B: LDA $DC0D
	0xA9, 0x81,			// 3E: LDA #$81
	0x8D, 0x0D, 0xDC,	// 40: STA $DC0D
	//0x40,				// 43: RTI
	0xA5, 0x01,			// LDA $01
	0x29, 0x03,			// AND #$03
	0xC9, 0x02,			// CMP #$02
	0x90, 0x03,			// BCC +
	0x4C, 0x81, 0xEA,	// JMP $EA81
	0x40,				// +: RTI
};


SidPlayer::SidPlayer()
	: m6502(NULL)
	, mSid(NULL)
	, mMemory(NULL){
	// TODO: fill out
}

MusicPlayer *SidPlayerFactory()
{
	return new SidPlayer;
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
	NLOGV("SidPlayer", "Reset");

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

	mState = MusicPlayer::Prepare(fileName);

    int result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

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

	mDriverPage = 0x9F00;

	if (mFileHeader.version == 2) {
	    NLOGD("SidPlayer", "Reading PSID v2 header fields");
	    musicFile.read(((char*)&mFileHeader) + 0x76, sizeof(mFileHeader) - 0x76);
		if (!musicFile) {
			NLOGE("SidPlayer", "Reading PSID v2 header fields failed");
	        musicFile.close();
			return MusicPlayer::ERROR_FILE_IO;
		}
		mDriverPage = (uint16_t)mFileHeader.startPage << 8;
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

	if (mDriverPage == 0) {
		mDriverPage = (mFileHeader.loadAddress & 0xFF00) - 0x100;
		if (mDriverPage >= 0xD000) {
			mDriverPage = 0x8000 | (mDriverPage & 0xFFF);
		}
	}

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
	mMemory->SetCpu(m6502);
	m6502->SetMapper(mMemory);

	mMemory->Reset();
	m6502->Reset();
	mSid->Reset();

	NLOGE("SidPlayer", "Writing SID playback driver at %#x", mDriverPage);
	uint8_t *ram = mMemory->GetRamPointer();
	for (i = 0; i < sizeof(SID_DRIVER); i++) {
		ram[mDriverPage + i] = SID_DRIVER[i];
	}
	ram[mDriverPage + 0x32] = mFileHeader.initAddress & 0xFF;
	ram[mDriverPage + 0x33] = mFileHeader.initAddress >> 8;
	ram[mDriverPage + 0x37] = mDriverPage >> 8;
	ram[mDriverPage + 0x39] = mFileHeader.playAddress & 0xFF;
	ram[mDriverPage + 0x3A] = mFileHeader.playAddress >> 8;
	if ((mFileHeader.initAddress >= 0xE000) || mFileHeader.playAddress >= 0xE000) {
		ram[mDriverPage + 0x2C] = 0x35;
	}
	ram[0x314] = (mDriverPage + 0x38) & 0xFF;
	ram[0x315] = (mDriverPage + 0x38) >> 8;
	ram[0xFFFE] = (mDriverPage + 0x38) & 0xFF;
	ram[0xFFFF] = (mDriverPage + 0x38) >> 8;

	mMetaData.SetNumSubSongs(mFileHeader.numSongs);
	mMetaData.SetDefaultSong(mFileHeader.firstSong);

	m6502->mRegs.S = 0xFF;
	SetSubSong(mFileHeader.firstSong - 1);
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

	uint8_t *ram = mMemory->GetRamPointer();
	ram[mDriverPage + 0x30] = subSong;
	Execute6502(mDriverPage);
}


void SidPlayer::TimerIrq(uint8_t timerNum)
{
	NLOGV("SidPlayer", "TimerA IRQ");

	if (timerNum == Cia6526::TIMER_A) {
		m6502->mCycles = 0;
		m6502->Run(mFrameCycles);
	}
}


void SidPlayer::Execute6502(uint16_t address)
{
	NLOGV("SidPlayer", "Execute6502(%#x)", address);

	m6502->mRegs.PC = address;
	m6502->mCycles = 0;
	m6502->Run(mFrameCycles*4);
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
			//Execute6502(mFileHeader.playAddress);
		}

		mMemory->mCia[SidMapper::CIA1].mTimer[Cia6526::TIMER_A]->Step();
		mMemory->mVicII.Step();
		mSid->Step();

		for (i = 0; i < 3; i++) {
			out = mSid->mChannels[i].mVol & mSid->mChannels[i].mOutputMask;

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


