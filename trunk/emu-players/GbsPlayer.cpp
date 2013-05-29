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

#include <FBase.h>
#include <iostream>
#include <fstream>
#include <string>
#include "GbsPlayer.h"
#include "GbCommon.h"


GbsPlayer::GbsPlayer()
{
	cart = NULL;
}


int GbsPlayer::Reset()
{
	if (cart) delete [] cart;
	cart = NULL;

	if (mBlipBuf) delete mBlipBuf;
	mBlipBuf = NULL;
	if (mSynth) delete [] mSynth;
	mSynth = NULL;

	mState = MusicPlayer::STATE_CREATED;

	return 0;
}


int GbsPlayer::Prepare(std::wstring fileName)
{
	uint32_t  i;
    size_t fileSize;

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    std::ifstream musicFile(std::string(fileName.begin(), fileName.end()).c_str(),
    		std::ios::in | std::ios::binary);
    if (!musicFile) {
    	// AppLog("Failed to open file %S", fileName.c_str());
    	return -1;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    AppLog("Reading GBS header");
    musicFile.read((char*)&mFileHeader, 0x70);
    AppLog("ID: %c%c%c", mFileHeader.ID[0], mFileHeader.ID[1], mFileHeader.ID[2]);
    AppLog("Load: %#x\nInit: %#x\nPlay: %#x",
    		mFileHeader.loadAddress, mFileHeader.initAddress, mFileHeader.playAddress);

    numBanks = ((fileSize + mFileHeader.loadAddress - 0x70) + 0x3fff) >> 14;

    AppLog("Trying to allocate %d bytes", (uint32_t)numBanks << 14);

    cart = new unsigned char[(uint32_t)numBanks << 14];
	musicFile.read((char*)cart + mFileHeader.loadAddress, fileSize-0x70);
	if (!musicFile) {
		AppLog("Read failed");
		musicFile.close();
		return -1;
	}

	musicFile.close();
	return 0;
}


int GbsPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	memset(buffer, 0, numSamples*2*2);
	return 0;
}
