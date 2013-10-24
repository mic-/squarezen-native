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
#define NLOG_TAG "SndhPlayer"

#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include "NativeLogger.h"
#include "SndhPlayer.h"
#include "../unice/unice68.h"

#define BYTESWAP(w) w = (((w) & 0xFF00) >> 8) | (((w) & 0x00FF) << 8)

const std::string SndhPlayer::SNDH_SIGNATURE = "SNDH";
const std::string SndhPlayer::ICE_PACKER_SIGNATURE = "ICE!";


SndhPlayer::SndhPlayer()
	: m68k(NULL)
	, mYm(NULL)
	, mMemory(NULL)
	, mNumSongs(1)
{
}

MusicPlayer *SndhPlayerFactory()
{
	return new SndhPlayer;
}

SndhPlayer::~SndhPlayer()
{
	delete m68k;
	delete mYm;
	delete mMemory;

	m68k    = NULL;
	mYm     = NULL;
	mMemory = NULL;
}

MusicPlayer::Result SndhPlayer::Reset()
{
	// ToDo: implement
	NLOGV(NLOG_TAG, "Reset");

	delete m68k;
	delete mYm;
	delete mMemory;

	m68k    = NULL;
	mYm     = NULL;
	mMemory = NULL;

	mNumSongs = 1;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

char *SndhPlayer::ReadNumber(char *fileImage, int& num, size_t maxChars, int minVal, int maxVal)
{
	num = 0;
	int i = 0;

	while (i < maxChars && *fileImage) {
		if (*fileImage >= '0' && *fileImage <= '9') {
			num = (num * 10) + (*fileImage - '0');
		}
		fileImage++;
		i++;
	}
	while (*fileImage++);
	while (!(*fileImage)) fileImage++;
	if (num < minVal) num = minVal;
	if (num > maxVal) num = maxVal;
	return fileImage;
}


uint8_t *SndhPlayer::ParseTags(char *fileImage, size_t remainingBytes)
{
	bool headerEnd = false;
	char *endPointer = fileImage + remainingBytes;
	char * const initialPointer = fileImage;
	int n;

	while ((fileImage < endPointer) && !headerEnd) {
		NLOGV(NLOG_TAG, "ParseTags: fileImage = %p, tag = %c%c%c%c", fileImage,
				fileImage[0], fileImage[1], fileImage[2], fileImage[3]);

		if (strncmp(fileImage, "HDNS", 4) == 0) {
			fileImage += 5;			// Skip past the tag + null terminator
			headerEnd = true;

		} else if (strncmp(fileImage, "TITL", 4) == 0) {
			fileImage += 4;
			mMetaData.SetTitle(fileImage);
			while (*fileImage++);

		} else if (strncmp(fileImage, "COMM", 4) == 0) {
			fileImage += 4;
			mMetaData.SetAuthor(fileImage);
			while (*fileImage++);

		} else if (strncmp(fileImage, "RIPP", 4) == 0) {	// Ripper name
			fileImage += 4;
			mMetaData.SetComment(fileImage);
			while (*fileImage++);

		} else if (strncmp(fileImage, "CONV", 4) == 0) {	// Converter name
			fileImage += 4;
			while (*fileImage++);	// Skip the string

		} else if (strncmp(fileImage, "YEAR", 4) == 0) {
			fileImage += 4;
			while (*fileImage++);	// Skip the string

		// Song lengths (one 16-bit word per sub-tune, expressed in seconds)
		} else if (strncmp(fileImage, "TIME", 4) == 0) {
			fileImage += 4;
			uint16_t *length = (uint16_t*)fileImage;
			mSongLength.clear();
			for (int i = 0; i < mNumSongs; i++) {
				uint16_t len = *length++;
				BYTESWAP(len);
				mSongLength.push_back(len);
			}
			fileImage = (char*)length;

		// Song name offsets (one 16-bit word per sub-tune, relative to the start of the !#SN tag)
		} else if (strncmp(fileImage, "!#SN", 4) == 0) {
			uint16_t tagOffset = (uint16_t)(fileImage - initialPointer);
			fileImage += 4;
			uint16_t *offset = (uint16_t*)fileImage;
			mSongNameOffset.clear();
			for (int i = 0; i < mNumSongs; i++) {
				uint16_t offs = *offset++;
				BYTESWAP(offs);
				mSongNameOffset.push_back(offs + tagOffset);
			}
			fileImage = (char*)offset;

		// Number of sub-tunes (two digits, 1-99)
		} else if (strncmp(fileImage, "##", 2) == 0) {
			fileImage += 2;
			fileImage = ReadNumber(fileImage, n, 2, 0, 99);
			mNumSongs = n;
			mSongLength.clear();
			mSongNameOffset.clear();

		// The default song to play (two digits, 1-99)
		} else if (strncmp(fileImage, "!#", 2) == 0) {
			fileImage += 2;
			fileImage = ReadNumber(fileImage, n, 2, 0, 99);
			mMetaData.SetDefaultSong(n);

		} else if (strncmp(fileImage, "!V", 2) == 0) {
			fileImage += 2;
			fileImage = ReadNumber(fileImage, n, 2, 0, 99);
			// Doesn't actually change the VBL frequency of the machine
			mVblFrequency = n;

		} else if (strncmp(fileImage, "TA", 2) == 0) {
			fileImage += 2;
			fileImage = ReadNumber(fileImage, n, 3, 0, 999);
			mTimerFrequency[0] = n;
		} else if (strncmp(fileImage, "TB", 2) == 0) {
			fileImage += 2;
			fileImage = ReadNumber(fileImage, n, 3, 0, 999);
			mTimerFrequency[1] = n;
		} else if (strncmp(fileImage, "TC", 2) == 0) {
			fileImage += 2;
			fileImage = ReadNumber(fileImage, n, 3, 0, 999);
			mTimerFrequency[2] = n;
		} else if (strncmp(fileImage, "TD", 2) == 0) {
			fileImage += 2;
			fileImage = ReadNumber(fileImage, n, 3, 0, 999);
			mTimerFrequency[3] = n;

		// ToDo: handle all other supported tags

		} else {
			NLOGE(NLOG_TAG, "Unsupported tag found: %x%x%x%x", fileImage[0], fileImage[1], fileImage[2], fileImage[3]);
			return NULL;
		}
	}

	return (headerEnd ? (uint8_t*)fileImage : NULL);
}


MusicPlayer::Result SndhPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    mMemory = new SndhMapper(fileSize);
    if (!mMemory) {
    	NLOGE(NLOG_TAG, "Failed to allocate memory for SNDH file");
    	musicFile.close();
    	return MusicPlayer::ERROR_OUT_OF_MEMORY;
    }
    uint8_t *fileImage = mMemory->GetFileImagePointer();

    NLOGV(NLOG_TAG, "Reading file");
    musicFile.read((char*)fileImage, fileSize);
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading SNDH file failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	musicFile.close();

	SndhFileHeader *header = (SndhFileHeader*)fileImage;

	// First check if this is a compressed file
	if (ICE_PACKER_SIGNATURE.compare(0, 4, (char*)fileImage, 4) == 0) {
		int depackedSize = unice68_get_depacked_size(fileImage, NULL);
		if (depackedSize <= 0) {
			NLOGE(NLOG_TAG, "Malformed ICE header");
			return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
		}
		NLOGD(NLOG_TAG, "Depacked size: %d bytes", depackedSize);
		SndhMapper *newMapper = new SndhMapper(depackedSize);
		if (unice68_depacker(newMapper->GetFileImagePointer(), fileImage)) {
			NLOGE(NLOG_TAG, "ICE depacking failed");
			delete newMapper;
			return MusicPlayer::ERROR_DECOMPRESSION;
		}
		delete mMemory;			// delete the old mapper
		mMemory = newMapper;	// make the new mapper the current one
		newMapper = NULL;
		fileImage = mMemory->GetFileImagePointer();
		header = (SndhFileHeader*)fileImage;
		fileSize = depackedSize;
	}

    if (SNDH_SIGNATURE.compare(0, 4, header->signature, 4)) {
		NLOGE(NLOG_TAG, "No SNDH or ICE! signature");
		return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

	mNumSongs = 1;
    if (NULL != ParseTags((char*)fileImage + sizeof(SndhFileHeader), fileSize - sizeof(SndhFileHeader))) {
    	NLOGD(NLOG_TAG, "Malformed tag(s) found, or no HDNS tag found");
        musicFile.close();
		return result;
    }

    if (mSongLength.empty()) {
    	mNumSongs = 1;
    	mSongLength.push_back(0);
    	NLOGD(NLOG_TAG, "No ##nn tag found; assuming a single sub-tune with infinite length");
    }
    if (mSongNameOffset.empty()) {
    	// ToDo: initialize mSongNameOffset ?
    }
    mMetaData.SetNumSubSongs(mNumSongs);

    m68k = new M68000;
    mYm = new YmChip(YmChip::YM2149_ENVELOPE_STEPS);
    if (!m68k || !mYm) {
    	NLOGE(NLOG_TAG, "Failed to create M68k or YM2149 emulators");
    	return MusicPlayer::ERROR_OUT_OF_MEMORY;
    }

	m68k->Reset();
	mYm->Reset();
	mMemory->Reset();

    // ToDo: finish

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SndhPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD(NLOG_TAG, "SetSubSong(%d)", subSong);
	// ToDo: implement
	mMetaData.SetLengthMs(mSongLength[subSong] * 1000);
	mMetaData.SetSubTitle((char*)(mMemory->GetFileImagePointer()) + mSongNameOffset[subSong]);
}


MusicPlayer::Result SndhPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		// ToDo: run the m68k at appropriate intervals

		mYm->Step();

		for (i = 0; i < 3; i++) {
			out = (mYm->mChannels[i].mPhase | mYm->mChannels[i].mToneOff) &
				  (mYm->mNoise.mOut         | mYm->mChannels[i].mNoiseOff);
			out = (-out) & *(mYm->mChannels[i].mCurVol);

			if (out != mYm->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mYm->mChannels[i].mOut = out;
			}
		}
	}

	mBlipBuf->end_frame(blipLen);
	//PresentBuffer(buffer, mBlipBuf);

	return MusicPlayer::OK;
}


void SndhPlayer::GetChannelOutputs(int16_t *outputs) const
{
	outputs[0] = mYm->mChannels[0].mOut;
	outputs[1] = mYm->mChannels[1].mOut;
	outputs[2] = mYm->mChannels[2].mOut;
}

