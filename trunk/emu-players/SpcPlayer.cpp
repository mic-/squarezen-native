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
{

}

SpcPlayer::~SpcPlayer()
{

}

int SpcPlayer::Reset()
{
	// ToDo: implement
	NLOGV("SpcPlayer", "SpcPlayer::Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

int SpcPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("SpcPlayer", "SpcPlayer::Prepare(%s)", fileName.c_str());

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NLOGE("SpcPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

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

	mMemory->Reset();
	mSSmp->Reset();
	//mSDsp->Reset();

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


int SpcPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}
