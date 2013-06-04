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
#include <string>
#include <string.h>
#include "NativeLogger.h"
#include "GbsPlayer.h"
#include "GbCommon.h"
#include "GbMemory.h"
#include "GbZ80.h"


GbsPlayer::GbsPlayer()
	: mBlipBufRight(NULL), mSynthRight(NULL)
{
	cart = NULL;
}


int GbsPlayer::Reset()
{
	delete [] cart;
	cart = NULL;

	delete mBlipBuf;
	mBlipBuf = NULL;
	delete [] mSynth;
	mSynth = NULL;

	delete mBlipBufRight;
	mBlipBufRight = NULL;
	delete [] mSynthRight;
	mSynthRight = NULL;

	mState = MusicPlayer::STATE_CREATED;

	return 0;
}


int GbsPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
    size_t fileSize;

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	// NativeLog("Failed to open file %S", fileName.c_str());
    	return -1;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    musicFile.read((char*)&mFileHeader, 0x70);
    NativeLog(0, "GbsPlayer", "Reading GBS header");
    NativeLog(0, "GbsPlayer", "ID: %c%c%c", mFileHeader.ID[0], mFileHeader.ID[1], mFileHeader.ID[2]);
    NativeLog(0, "GbsPlayer", "Load: %#x\nInit: %#x\nPlay: %#x",
    		mFileHeader.loadAddress, mFileHeader.initAddress, mFileHeader.playAddress);
    NativeLog(0, "GbsPlayer", "Timer control %#x, timer modulo %#x", mFileHeader.timerCtrl, mFileHeader.timerMod);

    numBanks = ((fileSize + mFileHeader.loadAddress - 0x70) + 0x3fff) >> 14;

    NativeLog(0, "GbsPlayer", "Trying to allocate %d bytes (filesize = %d)", (uint32_t)numBanks << 14, fileSize);

    cart = new unsigned char[(uint32_t)numBanks << 14];

    memset(cart, 0, (uint32_t)numBanks << 4);
	musicFile.read((char*)cart + mFileHeader.loadAddress, fileSize-0x70);
	if (!musicFile) {
		NativeLog(0, "GbsPlayer", "Read failed");
        musicFile.close();
		return -1;
	}

	NativeLog(0, "GbsPlayer.h", "File read done");

	musicFile.close();

	{
		mBlipBuf = new Blip_Buffer();
		mSynth = new Blip_Synth<blip_low_quality,82>[4];

		if (mBlipBuf->set_sample_rate(44100)) {
			//NativeLog("Failed to set blipbuffer sample rate");
			return -1;
		}
		mBlipBuf->clock_rate(DMG_CLOCK/2);

		for (i = 0; i < 4; i++) {
			//mSynth[i].volume(0.22);
			mSynth[i].output(mBlipBuf);
		}
	}
	{
		mBlipBufRight = new Blip_Buffer();
		mSynthRight = new Blip_Synth<blip_low_quality,82>[4];

		if (mBlipBufRight->set_sample_rate(44100)) {
			//NativeLog("Failed to set blipbuffer sample rate");
			return -1;
		}
		mBlipBufRight->clock_rate(DMG_CLOCK/2);

		for (i = 0; i < 4; i++) {
			//mSynthRight[i].volume(0.22);
			mSynthRight[i].output(mBlipBufRight);
		}
	}

	SetMasterVolume(0, 0);

	mFrameCycles = (DMG_CLOCK / 2) / 60;
	mCycleCount = 0;

	mem_set_papu(&mPapu);
	mem_set_gbsplayer(this);
	mem_reset();
	cpu_reset();
	mPapu.Reset();

	NativeLog(0, "GbsPlayer", "Reset done");

	cart[0xF0] = 0xCD;	// CALL nn nn
	cart[0xF1] = mFileHeader.initAddress & 0xFF;
	cart[0xF2] = mFileHeader.initAddress >> 8;
	cart[0xF3] = 0x76;	// HALT
	cpu.regs.PC = 0xF0;
	cpu.regs.SP = mFileHeader.SP;
	cpu.regs.A = 1; // song number
	cpu.cycles = 0;
	cpu_execute(mFrameCycles);

	NativeLog(0, "GbsPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;

	return 0;
}


void GbsPlayer::SetMasterVolume(int left, int right)
{
	for (int i = 0; i < 4; i++) {
		mSynth[i].volume(0.0314f * (float)left);
		mSynthRight[i].volume(0.0314f * (float)right);
	}
}


void GbsPlayer::PresentBuffer(int16_t *out, Blip_Buffer *inL, Blip_Buffer *inR)
{
	int count = inL->samples_avail();

	inL->read_samples(out, count, 1);
	inR->read_samples(out+1, count, 1);
}


int GbsPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int k;
	int blipLen = mBlipBuf->count_clocks(numSamples);
	int16_t out, outL, outR;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return -1;
    }

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			//NativeLog("Running GBZ80 %d cycles", mFrameCycles);
			cpu.halted = 0;
			cart[0xF0] = 0xCD;	// CALL nn nn
			cart[0xF1] = mFileHeader.playAddress & 0xFF;
			cart[0xF2] = mFileHeader.playAddress >> 8;
			cart[0xF3] = 0x76;	// HALT
			cpu.regs.PC = 0xF0;
			cpu.cycles = 0;
			cpu_execute(mFrameCycles);
		}

		mPapu.Step();

		for (int i = 0; i < 4; i++) {
			out = (-mPapu.mChannels[i].mPhase) &
				  GbPapuChip::VOL_TB[mPapu.mChannels[i].mCurVol & 0x0F];
			outL = out & mPapu.ChannelEnabledLeft(i);
			outR = out & mPapu.ChannelEnabledRight(i);

			if (outL != mPapu.mChannels[i].mOutL) {
				mSynth[i].update(k, outL);
				mPapu.mChannels[i].mOutL = outL;
			}
			if (outR != mPapu.mChannels[i].mOutR) {
				mSynthRight[i].update(k, outR);
				mPapu.mChannels[i].mOutR = outR;
			}
		}

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	mBlipBufRight->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf, mBlipBufRight);

	return 0;
}
