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
#define NLOG_TAG "FcPlayer"

#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "FcPlayer.h"

#define BYTESWAP(w) w = (((w) & 0xFF00) >> 8) | (((w) & 0x00FF) << 8)
#define WORDSWAP(d) d = (((d) & 0xFFFF0000) >> 16) | (((d) & 0x0000FFFF) << 16)

const std::string FcPlayer::FC13_ID = "SMOD";
const std::string FcPlayer::FC14_ID = "FC14";


FcPlayer::FcPlayer()
	: mIsFc14(false)
	, mFileHeader(NULL)
	, mSequences(NULL)
	, mPatterns(NULL)
{
}


MusicPlayer *FcPlayerFactory()
{
	return new FcPlayer;
}


FcPlayer::~FcPlayer()
{
	if (mIsFc14) {
		delete (Fc14FileHeader*)mFileHeader;
	} else {
		delete (Fc13FileHeader*)mFileHeader;
	}
	delete mSequences;
	delete mPatterns;
}


MusicPlayer::Result FcPlayer::Reset()
{
	// ToDo: implement
	NLOGV(NLOG_TAG, "Reset");

	if (mIsFc14) {
		delete (Fc14FileHeader*)mFileHeader;
	} else {
		delete (Fc13FileHeader*)mFileHeader;
	}
	delete mSequences;
	delete mPatterns;

	mFileHeader = NULL;
	mSequences = NULL;
	mPatterns = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

static void ByteswapUint32(uint32_t *u)
{
	uint16_t *p16;
	uint32_t littleEndian = 1;
	if (*(uint8_t*)&littleEndian == 1) {
		p16 = (uint16_t*)u;
		BYTESWAP(*p16);
		p16++;
		BYTESWAP(*p16);
		WORDSWAP(*u);
	}
}


// The header fields are big-endian.
// Change the byte order to little-endian if we're running
// on a little-endian machine.
static void ByteswapFc13Header(FcPlayer::Fc13FileHeader *header)
{
	uint32_t littleEndian = 1;
	if (*(uint8_t*)&littleEndian == 1) {
		ByteswapUint32(&(header->sequenceLength));
		ByteswapUint32(&(header->patternOffset));
		ByteswapUint32(&(header->patternLength));
		ByteswapUint32(&(header->freqSequenceOffset));
		ByteswapUint32(&(header->freqSequenceLength));
		ByteswapUint32(&(header->volSequenceOffset));
		ByteswapUint32(&(header->volSequenceLength));
		ByteswapUint32(&(header->sampleDataOffset));
		ByteswapUint32(&(header->sampleDataLength));
		for (int i = 0; i < 10; i++) {
			BYTESWAP(header->sampleInfo[i].length);
			BYTESWAP(header->sampleInfo[i].loopStart);
			BYTESWAP(header->sampleInfo[i].loopLength);
		}
	}
}


// The header fields are big-endian.
// Change the byte order to little-endian if we're running
// on a little-endian machine.
static void ByteswapFc14Header(FcPlayer::Fc14FileHeader *header)
{
	uint32_t littleEndian = 1;
	if (*(uint8_t*)&littleEndian == 1) {
		ByteswapUint32(&(header->sequenceLength));
		ByteswapUint32(&(header->patternOffset));
		ByteswapUint32(&(header->patternLength));
		ByteswapUint32(&(header->freqSequenceOffset));
		ByteswapUint32(&(header->freqSequenceLength));
		ByteswapUint32(&(header->volSequenceOffset));
		ByteswapUint32(&(header->volSequenceLength));
		ByteswapUint32(&(header->sampleDataOffset));
		ByteswapUint32(&(header->waveTableOffset));
		for (int i = 0; i < 10; i++) {
			BYTESWAP(header->sampleInfo[i].length);
			BYTESWAP(header->sampleInfo[i].loopStart);
			BYTESWAP(header->sampleInfo[i].loopLength);
		}
	}
}

MusicPlayer::Result FcPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    char id[4];
    NLOGV(NLOG_TAG, "Reading signature");
    musicFile.read(id, 4);
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading FC header ID failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	if (FC13_ID.compare(0, 4, id, 4) == 0) {
		NLOGD(NLOG_TAG, "Reading FC1.3 header");
		mFileHeader = new Fc13FileHeader;
		musicFile.read((char*)mFileHeader, sizeof(Fc13FileHeader) - 4);
		ByteswapFc13Header((Fc13FileHeader*)mFileHeader);
	} else if (FC14_ID.compare(0, 4, id, 4) == 0) {
		NLOGD(NLOG_TAG, "Reading FC1.4 header");
		mIsFc14 = true;
		mFileHeader = new Fc14FileHeader;
		musicFile.read((char*)mFileHeader, sizeof(Fc14FileHeader) - 4);
		ByteswapFc14Header((Fc14FileHeader*)mFileHeader);
	} else {
		NLOGE(NLOG_TAG, "Unsupported Future Composer type: %c%c%c%c", id[0], id[1], id[2], id[3]);
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
	}

	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading the FC header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	// Read sequences
	NLOGD(NLOG_TAG, "Allocating space for %d sequences", ((Fc13FileHeader*)mFileHeader)->sequenceLength / 13);
	mSequences = new std::vector<FcSequence>(((Fc13FileHeader*)mFileHeader)->sequenceLength / 13);
	mCurrSequence = 0;

	 for (std::vector<FcSequence>::iterator it = mSequences->begin();
			 it != mSequences->end(); ++it) {
		musicFile.read((char*)&(*it), sizeof(FcSequence));
		if (!musicFile) {
			NLOGE(NLOG_TAG, "Reading FC sequence data failed");
	        musicFile.close();
			return MusicPlayer::ERROR_FILE_IO;
		}
	}

	// Read patterns
	NLOGD(NLOG_TAG, "Allocating space for %d patterns", ((Fc13FileHeader*)mFileHeader)->patternLength / sizeof(FcPattern));
	mPatterns = new std::vector<FcPattern>(((Fc13FileHeader*)mFileHeader)->patternLength / sizeof(FcPattern));
	musicFile.seekg(((Fc13FileHeader*)mFileHeader)->patternOffset, musicFile.beg);

	 for (std::vector<FcPattern>::iterator it = mPatterns->begin();
			 it != mPatterns->end(); ++it) {
		musicFile.read((char*)&(*it), sizeof(FcPattern));
		if (!musicFile) {
			NLOGE(NLOG_TAG, "Reading FC pattern data failed");
	        musicFile.close();
			return MusicPlayer::ERROR_FILE_IO;
		}
	}

    // ToDo: finish

	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


MusicPlayer::Result FcPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}

void FcPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}
